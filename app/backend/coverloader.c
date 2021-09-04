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

static void img_loader_complete_cb(coverloader_req_t *req, SDL_Surface *data);

static void img_loader_fail_cb(coverloader_req_t *req);

static void img_loader_cancel_cb(coverloader_req_t *req);

static void memcache_item_free(lv_sdl_img_src_t *item);

static void coverloader_req_free(coverloader_req_t *req);

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
        .complete_cb = (img_loader_fn2) img_loader_complete_cb,
        .fail_cb = (img_loader_fn) img_loader_fail_cb,
        .cancel_cb = (img_loader_fn) img_loader_cancel_cb,
};

static img_loader_t *base_loader;
static lv_lru_t *mem_cache;

MAIN_THREAD void coverloader_init() {
    mem_cache = lv_lru_new(1024 * 1024 * 32, 720 * 1024, (lv_lru_free_t *) memcache_item_free, SDL_free);
    base_loader = img_loader_create(&coverloader_impl);
}

MAIN_THREAD void coverloader_destroy() {
    img_loader_destroy(base_loader);
    lv_lru_free(mem_cache);
}

MAIN_THREAD void coverloader_display(PSERVER_LIST node, int id, lv_obj_t *target, lv_coord_t target_width,
                                     lv_coord_t target_height) {
    coverloader_req_t *req = malloc(sizeof(coverloader_req_t));
    SDL_memset(req, 0, sizeof(coverloader_req_t));
    req->id = id;
    req->node = node;
    req->target = target;
    req->target_width = target_width;
    req->target_height = target_height;
    req->finished = false;
    req->mutex = SDL_CreateMutex();
    req->cond = SDL_CreateCond();
    img_loader_load(base_loader, req, &coverloader_cb);
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
    lv_lru_get(mem_cache, &req->id, sizeof(int), (void **) &result);
    req->result = result;
    return NULL;
}

static void coverloader_memcache_put(coverloader_req_t *req, SDL_Surface *cached) {
    lv_disp_t *disp = lv_disp_get_default();
    SDL_Renderer *renderer = disp->driver->user_data;
    lv_sdl_img_src_t *result = SDL_malloc(sizeof(lv_sdl_img_src_t));
    SDL_memset(result, 0, sizeof(lv_sdl_img_src_t));
    result->type = LV_SDL_IMG_TYPE_TEXTURE;
    result->cf = LV_IMG_CF_TRUE_COLOR;
    result->w = cached->w;
    result->h = cached->h;
    result->data.texture = SDL_CreateTextureFromSurface(renderer, cached);
    lv_lru_set(mem_cache, &req->id, sizeof(int), result, result->w * result->h);
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

static void img_loader_complete_cb(coverloader_req_t *req, SDL_Surface *data) {
    const void *old_src = lv_obj_get_style_bg_img_src(req->target, 0);
    if (old_src) {
        SDL_free((void *) old_src);
    }
    char *src = SDL_malloc(COVER_SRC_LEN_MAX);
    lv_sdl_img_src_stringify(req->result, src);
    lv_obj_set_style_bg_img_src(req->target, src, 0);
    lv_obj_set_style_bg_opa(req->target, 0, 0);
    coverloader_req_free(req);
}

static void img_loader_fail_cb(coverloader_req_t *req) {
    lv_obj_set_style_bg_img_src(req->target, NULL, 0);
    lv_obj_set_style_bg_opa(req->target, LV_OPA_COVER, 0);
    coverloader_req_free(req);
}

static void img_loader_cancel_cb(coverloader_req_t *req) {
    coverloader_req_free(req);
}

static void memcache_item_free(lv_sdl_img_src_t *item) {
    SDL_DestroyTexture(item->data.texture);
    SDL_free(item);
}

static void coverloader_req_free(coverloader_req_t *req) {
    SDL_DestroyMutex(req->mutex);
    SDL_DestroyCond(req->cond);
    SDL_free(req);
}