#include "coverloader.h"
#include "app.h"
#include "appitem.view.h"

#include <stddef.h>

#include "util/bus.h"
#include "util/path.h"

#include "libgamestream/client.h"
#include "libgamestream/errors.h"

#include <SDL_image.h>
#include "util/img_loader.h"
#include <gpu/sdl/lv_gpu_sdl_lru.h>
#include "util/refcounter.h"

#include "util/logging.h"
#include "res.h"

typedef struct coverloader_request {
    coverloader_t *loader;
    int id;
    PSERVER_LIST node;
    lv_obj_t *target;
    lv_coord_t target_width, target_height;
    lv_sdl_img_src_t *result;
    bool finished;
    img_loader_task_t *task;
    struct coverloader_request *prev;
    struct coverloader_request *next;
} coverloader_req_t;

typedef struct coverloader_memcache_key_t {
    int id;
    lv_coord_t target_width, target_height;
} coverloader_memcache_key_t;

#define LINKEDLIST_IMPL
#define LINKEDLIST_TYPE coverloader_req_t
#define LINKEDLIST_PREFIX reqlist
#define LINKEDLIST_DOUBLE 1

#include "util/linked_list.h"

#undef LINKEDLIST_TYPE
#undef LINKEDLIST_PREFIX
#undef LINKEDLIST_DOUBLE
#define LINKEDLIST_IMPL


static char *coverloader_cache_dir();

static void coverloader_cache_item_path(char path[4096], const coverloader_req_t *req);

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

static int reqlist_find_by_target(coverloader_req_t *p, const void *v);

static bool cover_is_placeholder(const SDL_Surface *surface);

static void target_deleted_cb(lv_event_t *e);

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
        .complete_cb = (img_loader_fn2) img_loader_result_cb,
        .fail_cb = (img_loader_fn) img_loader_result_cb,
        .cancel_cb = (img_loader_fn) img_loader_cancel_cb,
};

struct coverloader_t {
    img_loader_t *base_loader;
    lv_lru_t *mem_cache;
    GS_CLIENT client;
    coverloader_req_t *reqlist;
    SDL_Texture *defcover;
    refcounter_t refcounter;
};


coverloader_t *coverloader_new() {
    coverloader_t *loader = malloc(sizeof(coverloader_t));
    refcounter_init(&loader->refcounter);
    loader->mem_cache = lv_lru_new(1024 * 1024 * 32, 720 * 1024, (lv_lru_free_t *) memcache_item_free, NULL);
    loader->base_loader = img_loader_create(&coverloader_impl);
    loader->client = app_gs_client_new();
    loader->reqlist = NULL;

    lv_disp_t *disp = lv_disp_get_default();
    SDL_Renderer *renderer = disp->driver->user_data;
    loader->defcover = IMG_LoadTexture_RW(renderer, SDL_RWFromConstMem(res_default_cover_data, res_default_cover_size),
                                          1);
    return loader;
}

void coverloader_unref(coverloader_t *loader) {
    if (!refcounter_unref(&loader->refcounter)) {
        return;
    }
    if (loader->defcover) {
        SDL_DestroyTexture(loader->defcover);
    }
    gs_destroy(loader->client);
    img_loader_destroy(loader->base_loader);
    lv_lru_free(loader->mem_cache);
    refcounter_destroy(&loader->refcounter);
    free(loader);
}

void coverloader_display(coverloader_t *loader, PSERVER_LIST node, int id, lv_obj_t *target, lv_coord_t target_width,
                         lv_coord_t target_height) {
    coverloader_req_t *existing = reqlist_find_by(loader->reqlist, target, reqlist_find_by_target);
    if (existing && existing->task) {
        img_loader_cancel(loader->base_loader, existing->task);
    }

    coverloader_req_t *req = reqlist_new();
    req->loader = loader;
    req->id = id;
    req->node = node;
    req->target = target;
    req->target_width = target_width;
    req->target_height = target_height;
    req->finished = false;
    lv_obj_add_event_cb(target, target_deleted_cb, LV_EVENT_DELETE, req);
    loader->reqlist = reqlist_append(loader->reqlist, req);
    refcounter_ref(&loader->refcounter);
    img_loader_task_t *task = img_loader_load(loader->base_loader, req, &coverloader_cb);
    /* If no task returned, then the request has been freed already */
    if (!task) return;
    req->task = task;
}

static char *coverloader_cache_dir() {
    return path_cache();
}

static void coverloader_cache_item_path(char path[4096], const coverloader_req_t *req) {
    char *cachedir = coverloader_cache_dir();
    char basename[128];
    SDL_snprintf(basename, 128, "%s_%d", req->node->server->uuid, req->id);
    path_join_to(path, 4096, cachedir, basename);
    free(cachedir);
}

static SDL_Surface *coverloader_memcache_get(coverloader_req_t *req) {
    // Uses result cache instead
    lv_sdl_img_src_t *result = NULL;
    coverloader_memcache_key_t key = {
            .id = req->id,
            .target_width = req->target_width,
            .target_height = req->target_height
    };
    lv_lru_get(req->loader->mem_cache, &key, sizeof(key), (void **) &result);
    req->result = result;
    // Must return something not falsy or loader will think this is a cache miss
    return (SDL_Surface *) (size_t) (result != NULL);
}

static void coverloader_memcache_put(coverloader_req_t *req, SDL_Surface *cached) {
    lv_sdl_img_src_t *result = NULL;
    coverloader_memcache_key_t key = {
            .id = req->id,
            .target_width = req->target_width,
            .target_height = req->target_height
    };
    lv_lru_get(req->loader->mem_cache, &key, sizeof(key), (void **) &result);
    if (!result) {
        lv_disp_t *disp = lv_disp_get_default();
        SDL_Renderer *renderer = disp->driver->user_data;
        result = malloc(sizeof(lv_sdl_img_src_t));
        SDL_memset(result, 0, sizeof(lv_sdl_img_src_t));
        result->type = LV_SDL_IMG_TYPE_TEXTURE;
        result->cf = LV_IMG_CF_TRUE_COLOR;
        result->w = cached->w;
        result->h = cached->h;
        result->data.texture = SDL_CreateTextureFromSurface(renderer, cached);
        lv_lru_set(req->loader->mem_cache, &key, sizeof(key), result, result->w * result->h);
    }
    req->result = result;
    req->finished = true;
    SDL_FreeSurface(cached);
}

static SDL_Surface *coverloader_filecache_get(coverloader_req_t *req) {
    char path[4096];
    coverloader_cache_item_path(path, req);
    SDL_Surface *decoded = IMG_Load(path);
    if (!decoded) return NULL;
    if (cover_is_placeholder(decoded)) {
        SDL_FreeSurface(decoded);
        return NULL;
    }
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

static bool cover_is_placeholder(const SDL_Surface *surface) {
    return (surface->w == 130 && surface->h == 180) || (surface->w == 628 && surface->h == 888);
}

static void coverloader_filecache_put(coverloader_req_t *req, SDL_Surface *cached) {
    // This was done in fetch step
}

static SDL_Surface *coverloader_fetch(coverloader_req_t *req) {
    char path[4096];
    coverloader_cache_item_path(path, req);
    if (gs_download_cover(req->loader->client, (PSERVER_DATA) req->node->server, req->id, path) != GS_OK) {
        return NULL;
    }
    return coverloader_filecache_get(req);
}

static void coverloader_run_on_main(img_loader_t *loader, img_loader_fn fn, void *args) {
    (void) loader;
    bus_pushaction(fn, args);
}

static void img_loader_start_cb(coverloader_req_t *req) {
    appitem_viewholder_t *holder = req->target->user_data;
    lv_sdl_img_src_t src = {
            .w = req->target_width,
            .h = req->target_height,
            .type = LV_SDL_IMG_TYPE_TEXTURE,
            .cf = LV_IMG_CF_TRUE_COLOR,
            .data.texture = req->loader->defcover,
    };
    lv_sdl_img_src_stringify(&src, holder->cover_src);
    lv_img_set_src(req->target, holder->cover_src);
    lv_obj_add_flag(holder->title, LV_OBJ_FLAG_HIDDEN);
}

static void img_loader_result_cb(coverloader_req_t *req) {
    req->task = NULL;
    coverloader_t *loader = req->loader;
    if (req->target) {
        lv_obj_remove_event_cb(req->target, target_deleted_cb);
    } else {
        goto done;
    }
    appitem_viewholder_t *holder = req->target->user_data;
    if (req->result) {
        lv_sdl_img_src_stringify(req->result, holder->cover_src);
        lv_obj_add_flag(holder->title, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_sdl_img_src_t src = {
                .w = req->target_width,
                .h = req->target_height,
                .type = LV_SDL_IMG_TYPE_TEXTURE,
                .cf = LV_IMG_CF_TRUE_COLOR,
                .data.texture = loader->defcover,
        };
        lv_sdl_img_src_stringify(&src, holder->cover_src);
        lv_obj_clear_flag(holder->title, LV_OBJ_FLAG_HIDDEN);
    }
    lv_img_set_src(req->target, holder->cover_src);
    done:
    loader->reqlist = reqlist_remove(loader->reqlist, req);
    coverloader_req_free(req);
    coverloader_unref(loader);
}

static void img_loader_cancel_cb(coverloader_req_t *req) {
    if (req->target) {
        lv_obj_remove_event_cb(req->target, target_deleted_cb);
    }
    req->task = NULL;
    coverloader_t *loader = req->loader;
    loader->reqlist = reqlist_remove(loader->reqlist, req);
    coverloader_req_free(req);
    coverloader_unref(loader);
}

static void memcache_item_free(lv_sdl_img_src_t *item) {
    SDL_DestroyTexture(item->data.texture);
    free(item);
}

static inline void coverloader_req_free(coverloader_req_t *req) {
    free(req);
}

static int reqlist_find_by_target(coverloader_req_t *p, const void *v) {
    return p->target != v;
}

static void target_deleted_cb(lv_event_t *e) {
    coverloader_req_t *req = lv_event_get_user_data(e);
    req->target = NULL;
}