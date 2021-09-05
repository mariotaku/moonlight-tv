#define _COVERLOADER_IMPL

#include "coverloader.h"
#include "app.h"

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include <sys/stat.h>

#include "util/bus.h"
#include "util/user_event.h"
#include "util/path.h"

#include "libgamestream/client.h"
#include "libgamestream/errors.h"

#include <SDL_image.h>
#include <util/img_loader.h>
#include <gpu/sdl/lv_gpu_sdl_texture_cache.h>
#include <gpu/sdl/lv_gpu_sdl_lru.h>

#include "util/logging.h"
#include "util/memlog.h"

#define COVER_SRC_LEN_MAX 64

typedef struct coverloader_request {
    coverloader_t *loader;
    int id;
    PSERVER_LIST node;
    lv_obj_t *target;
    lv_coord_t target_width, target_height;
    SDL_mutex *mutex;
    SDL_cond *cond;
    lv_sdl_img_src_t *result;
    bool finished;
} coverloader_req_t;

static char *coverloader_cache_dir();

static SDL_Surface *coverloader_memcache_get(coverloader_req_t *req);

static void coverloader_memcache_put(coverloader_req_t *req, SDL_Surface *cached);

static void coverloader_memcache_put_wait(coverloader_req_t *req);

static SDL_Surface *coverloader_filecache_get(coverloader_req_t *req);

static void coverloader_filecache_put(coverloader_req_t *req, SDL_Surface *cached);

static SDL_Surface *coverloader_fetch(coverloader_req_t *req);

static void coverloader_run_on_main(img_loader_t *loader, img_loader_fn fn, void *args);

static void img_loader_start_cb(coverloader_req_t *req);

static void img_loader_result_cb(coverloader_req_t *req);

static void img_loader_cancel_cb(coverloader_req_t *req);

static void memcache_item_free(lv_sdl_img_src_t *item);

static void coverloader_req_free(coverloader_req_t *req);

static void obj_free_res_cb(lv_event_t *event);

static const img_loader_impl_t coverloader_impl = {
        .memcache_get = (img_loader_get_fn) coverloader_memcache_get,
        .memcache_put = (img_loader_fn2) coverloader_memcache_put,
        .memcache_put_wait = (img_loader_fn) coverloader_memcache_put_wait,
        .filecache_get = (img_loader_get_fn) coverloader_filecache_get,
        .filecache_put = (img_loader_fn2) coverloader_filecache_put,
        .fetch = (img_loader_get_fn) coverloader_fetch,
        .run_on_main = coverloader_run_on_main,
};

static const img_loader_cb_t coverloader_cb = {
        .start_cb = (img_loader_fn) img_loader_start_cb,
        .complete_cb = (img_loader_fn2) img_loader_result_cb,
        .fail_cb = (img_loader_fn) img_loader_result_cb,
        .cancel_cb = (img_loader_fn) img_loader_cancel_cb,
};

struct coverloader_t {
    img_loader_t *base_loader;
    lv_lru_t *mem_cache;
};


coverloader_t *coverloader_new() {
    coverloader_t *loader = malloc(sizeof(coverloader_t));
    loader->mem_cache = lv_lru_new(1024 * 1024 * 32, 720 * 1024, (lv_lru_free_t *) memcache_item_free, NULL);
    loader->base_loader = img_loader_create(&coverloader_impl);
    return loader;
}

void coverloader_destroy(coverloader_t *loader) {
    img_loader_destroy(loader->base_loader);
    lv_lru_free(loader->mem_cache);
    free(loader);
}

void coverloader_display(coverloader_t *loader, PSERVER_LIST node, int id, lv_obj_t *target, lv_coord_t target_width,
                         lv_coord_t target_height) {
    coverloader_req_t *req = malloc(sizeof(coverloader_req_t));
    SDL_memset(req, 0, sizeof(coverloader_req_t));
    req->loader = loader;
    req->id = id;
    req->node = node;
    req->target = target;
    req->target_width = target_width;
    req->target_height = target_height;
    req->finished = false;
    req->mutex = SDL_CreateMutex();
    req->cond = SDL_CreateCond();
    img_loader_load(loader->base_loader, req, &coverloader_cb);
}

char *coverloader_cache_dir() {
    char *cachedir = SDL_getenv("XDG_CACHE_DIR"), *confdir = NULL;
    if (cachedir) {
        confdir = path_join(cachedir, "moonlight-tv");
    } else {
        cachedir = SDL_getenv("HOME");
        confdir = path_join(cachedir, ".moonlight-tv-covers");
    }
    if (access(confdir, F_OK) == -1) {
        if (errno == ENOENT) {
            mkdir(confdir, 0755);
        }
    }
    return confdir;
}

static SDL_Surface *coverloader_memcache_get(coverloader_req_t *req) {
    // Uses result cache instead
    lv_sdl_img_src_t *result = NULL;
    lv_lru_get(req->loader->mem_cache, &req->id, sizeof(int), (void **) &result);
    req->result = result;
    return NULL;
}

static void coverloader_memcache_put(coverloader_req_t *req, SDL_Surface *cached) {
    lv_disp_t *disp = lv_disp_get_default();
    SDL_Renderer *renderer = disp->driver->user_data;
    lv_sdl_img_src_t *result = malloc(sizeof(lv_sdl_img_src_t));
    SDL_memset(result, 0, sizeof(lv_sdl_img_src_t));
    result->type = LV_SDL_IMG_TYPE_TEXTURE;
    result->cf = LV_IMG_CF_TRUE_COLOR;
    result->w = cached->w;
    result->h = cached->h;
    result->data.texture = SDL_CreateTextureFromSurface(renderer, cached);
    lv_lru_set(req->loader->mem_cache, &req->id, sizeof(int), result, result->w * result->h);
    req->result = result;
    req->finished = true;
    SDL_CondSignal(req->cond);
    SDL_FreeSurface(cached);
}

static void coverloader_memcache_put_wait(coverloader_req_t *req) {
    while (!req->finished) {
        SDL_LockMutex(req->mutex);
        SDL_CondWait(req->cond, req->mutex);
        SDL_UnlockMutex(req->mutex);
    }
}

static SDL_Surface *coverloader_filecache_get(coverloader_req_t *req) {
    char *cachedir = coverloader_cache_dir();
    char path[4096];
    SDL_snprintf(path, 4096, "%s/%d", cachedir, req->id);
    free(cachedir);
    SDL_Surface *decoded = IMG_Load(path);
    if (!decoded) return NULL;
    SDL_Surface *scaled = SDL_CreateRGBSurface(0, req->target_width, req->target_height,
                                               decoded->format->BitsPerPixel, decoded->format->Rmask,
                                               decoded->format->Gmask, decoded->format->Bmask,
                                               decoded->format->Amask);
    double srcratio = decoded->w / (double) decoded->h, dstratio = req->target_width / (double) req->target_height;
    SDL_Rect srcrect;
    if (srcratio > dstratio) {
        // Source is wider than destination
        srcrect.h = decoded->h;
        srcrect.w = decoded->h * dstratio;
        srcrect.y = 0;
        srcrect.x = (decoded->w - srcrect.w) / 2;
    } else {
        // Destination is wider than source
        srcrect.w = decoded->w;
        srcrect.h = decoded->w / dstratio;
        srcrect.x = 0;
        srcrect.y = (decoded->h - srcrect.h) / 2;
    }
    SDL_BlitScaled(decoded, &srcrect, scaled, NULL);
    SDL_FreeSurface(decoded);
    return scaled;
}

static void coverloader_filecache_put(coverloader_req_t *req, SDL_Surface *cached) {
    // This was done in fetch step
}

static SDL_Surface *coverloader_fetch(coverloader_req_t *req) {
    char *cachedir = coverloader_cache_dir();
    char path[4096];
    SDL_snprintf(path, 4096, "%s/%d", cachedir, req->id);
    free(cachedir);
    if (gs_download_cover(app_gs_client_obtain(), (PSERVER_DATA) req->node->server, req->id, path) == GS_OK) {
        return coverloader_filecache_get(req);
    }
    return NULL;
}

static void coverloader_run_on_main(img_loader_t *loader, img_loader_fn fn, void *args) {
    (void) loader;
    bus_pushaction(fn, args);
}

static void img_loader_start_cb(coverloader_req_t *req) {

}

static void img_loader_result_cb(coverloader_req_t *req) {
    const void *old_src = lv_img_get_src(req->target);
    if (old_src) {
        free((void *) old_src);
        lv_obj_remove_event_cb(req->target, obj_free_res_cb);
    }
    if (req->result) {
        char *src = malloc(COVER_SRC_LEN_MAX);
        lv_sdl_img_src_stringify(req->result, src);
        lv_img_set_src(req->target, src);
        lv_obj_add_event_cb(req->target, obj_free_res_cb, LV_EVENT_DELETE, src);
    } else {
        lv_img_set_src(req->target, NULL);
    }
    coverloader_req_free(req);
}

static void img_loader_cancel_cb(coverloader_req_t *req) {
    coverloader_req_free(req);
}

static void memcache_item_free(lv_sdl_img_src_t *item) {
    SDL_DestroyTexture(item->data.texture);
    free(item);
}

static void coverloader_req_free(coverloader_req_t *req) {
    SDL_DestroyMutex(req->mutex);
    SDL_DestroyCond(req->cond);
    free(req);
}

static void obj_free_res_cb(lv_event_t *event) {
    free(event->user_data);
}