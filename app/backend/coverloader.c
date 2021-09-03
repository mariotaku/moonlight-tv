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
#include "util/lruc.h"
#include "util/path.h"

#include "libgamestream/client.h"
#include "libgamestream/errors.h"

#include <SDL_image.h>
#include <util/img_loader.h>

#include "util/logging.h"
#include "util/memlog.h"

typedef struct coverloader_request {
    int id;
    PSERVER_LIST node;
    lv_obj_t *target;
    lv_coord_t target_size;
} coverloader_req_t;

THREAD_SAFE static void coverloader_cache_item_free(void *p);

static char *coverloader_cache_dir();

static char *coverloader_memcache_get(coverloader_req_t *req);

static void coverloader_memcache_put(coverloader_req_t *req, char *cached);

static char *coverloader_filecache_get(coverloader_req_t *req);

static void coverloader_filecache_put(coverloader_req_t *req, char *cached);

static char *coverloader_fetch(coverloader_req_t *req);

static void coverloader_run_on_main(img_loader_t *loader, img_loader_fn fn, void *args);

static void img_loader_start_cb(coverloader_req_t *req);

static void img_loader_complete_cb(coverloader_req_t *req, char *data);

static void img_loader_fail_cb(coverloader_req_t *req);

static void img_loader_cancel_cb(coverloader_req_t *req);

static const img_loader_impl_t coverloader_impl = {
        .memcache_get = (img_loader_get_fn) coverloader_memcache_get,
        .memcache_put = (img_loader_fn2) coverloader_memcache_put,
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

static lruc *coverloader_mem_cache;
static img_loader_t *base_loader;

#define ITEM_AVERAGE_SIZE 360000

MAIN_THREAD void coverloader_init() {
    // 32MB limit, 720K each
    coverloader_mem_cache = lruc_new(32 * 1024 * 1024, ITEM_AVERAGE_SIZE, coverloader_cache_item_free);

    base_loader = img_loader_create(&coverloader_impl);
}

MAIN_THREAD void coverloader_destroy() {
    img_loader_destroy(base_loader);
    lruc_free(coverloader_mem_cache);
}

MAIN_THREAD void coverloader_display(PSERVER_LIST node, int id, lv_obj_t *target, lv_coord_t target_size) {
    coverloader_req_t *req = malloc(sizeof(coverloader_req_t));
    req->id = id;
    req->node = node;
    req->target = target;
    req->target_size = target_size;
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

void coverloader_cache_item_free(void *p) {
    lv_sdl_img_src_t *item = p;
    switch (item->type) {
        case LV_SDL_IMG_TYPE_SURFACE: {
            SDL_FreeSurface(item->data.surface);
            break;
        }
        default: {
            break;
        }
    }
    free(item);
}

static char *coverloader_memcache_get(coverloader_req_t *req) {
    char *cached = NULL;
    lruc_get(coverloader_mem_cache, req->id, (void **) &cached);
    return cached;
}

static void coverloader_memcache_put(coverloader_req_t *req, char *cached) {
    lv_sdl_img_src_t src;
    lv_sdl_img_src_parse(cached, &src);
    lruc_set(coverloader_mem_cache, req->id, cached, src.w * src.h);
}

static char *coverloader_filecache_get(coverloader_req_t *req) {
    char *cachedir = coverloader_cache_dir();
    char path[4096];
    SDL_snprintf(path, 4096, "%s/%d", cachedir, req->id);
    free(cachedir);
    SDL_Surface *surface = IMG_Load(path);
    if (!surface) return NULL;
    if (surface->format->BytesPerPixel != 4) {
        SDL_Surface *argb = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_ARGB8888, 0);
        SDL_FreeSurface(surface);
        surface = argb;
    }
    lv_sdl_img_src_t decoded;
    decoded.w = surface->w;
    decoded.h = surface->h;
    decoded.cf = LV_IMG_CF_TRUE_COLOR_ALPHA;
    decoded.type = LV_SDL_IMG_TYPE_SURFACE;
    decoded.data.surface = surface;
    char src[LV_SDL_IMG_LEN - 8];
    lv_sdl_img_src_stringify(&decoded, src);
    return SDL_strdup(src);
}

static void coverloader_filecache_put(coverloader_req_t *req, char *cached) {
    // This was done in fetch step
}

static char *coverloader_fetch(coverloader_req_t *req) {
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

static void img_loader_complete_cb(coverloader_req_t *req, char *data) {
    lv_obj_set_style_bg_img_src(req->target, data, 0);
}

static void img_loader_fail_cb(coverloader_req_t *req) {
    lv_obj_set_style_bg_img_src(req->target, NULL, 0);
}

static void img_loader_cancel_cb(coverloader_req_t *req) {

}