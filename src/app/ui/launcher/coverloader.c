#include "coverloader.h"
#include "app.h"
#include "appitem.view.h"

#include <stddef.h>

#include "util/bus.h"
#include "util/path.h"

#include "libgamestream/client.h"
#include "libgamestream/errors.h"

#include <SDL_image.h>
#include "draw/sdl/lv_draw_sdl.h"
#include "misc/lv_lru.h"
#include "util/img_loader.h"
#include "refcounter.h"

#include "res.h"

typedef struct memcache_key_t {
    int id;
    lv_coord_t target_width, target_height;
} memcache_key_t;

typedef struct memcache_item_t {
    lv_img_dsc_t src;
    lv_sdl_img_data_t data;
    lv_ll_t objs;
    lv_coord_t target_width, target_height, target_radius;
} memcache_item_t;

typedef struct img_loader_req_t {
    coverloader_t *loader;
    uuidstr_t server_id;
    int id;
    lv_obj_t *target;
    lv_coord_t target_width, target_height;
    memcache_item_t *src;
    bool finished;
    SDL_Surface *cached;
    img_loader_task_t *task;
    struct img_loader_req_t *prev;
    struct img_loader_req_t *next;
} coverloader_req_t;

#define LINKEDLIST_IMPL
#define LINKEDLIST_TYPE coverloader_req_t
#define LINKEDLIST_PREFIX reqlist
#define LINKEDLIST_DOUBLE 1

#include "linked_list.h"
#include "lazy.h"
#include "draw/sdl/lv_draw_sdl_img.h"
#include "logging.h"

#undef LINKEDLIST_TYPE
#undef LINKEDLIST_PREFIX
#undef LINKEDLIST_DOUBLE
#define LINKEDLIST_IMPL

#ifndef DEBUG
#define DEBUG 0
#endif

static char *coverloader_cache_dir();

static GS_CLIENT coverloader_gs_client(coverloader_t *loader);

static void coverloader_cache_item_path(char path[4096], const coverloader_req_t *req);

static bool coverloader_memcache_get(coverloader_req_t *req);

static void coverloader_memcache_put(coverloader_req_t *req);

static bool coverloader_filecache_get(coverloader_req_t *req);

static void coverloader_filecache_put(coverloader_req_t *req);

static bool coverloader_fetch(coverloader_req_t *req);

static void coverloader_run_on_main(img_loader_t *loader, img_loader_run_on_main_fn fn, void *args);

static void img_loader_start_cb(coverloader_req_t *req);

static void img_loader_result_cb(coverloader_req_t *req);

static void img_loader_cancel_cb(coverloader_req_t *req);

/**
 *
 * @param obj lv_img instance
 * @param src loaded image, NULL will fallback to default cover
 */
static void img_set_cover(lv_obj_t *obj, memcache_item_t *src);

static struct memcache_item_t *memcache_item_new();

static void memcache_item_free(memcache_item_t *item);

static void coverloader_req_free(coverloader_req_t *req);

static int reqlist_find_by_target(coverloader_req_t *p, const void *v);

static bool cover_is_placeholder(const SDL_Surface *surface);

static void target_deleted_cb(lv_event_t *e);

static void target_src_unlink_cb(lv_event_t *e);

static void memcache_item_ref_obj(memcache_item_t *item, lv_obj_t *obj);

static void memcache_item_unref_obj(memcache_item_t *item, const lv_obj_t *obj);

static void purge_img_cache(lv_draw_sdl_ctx_t *ctx, const memcache_item_t *item);

static void purge_corners_cache(lv_draw_sdl_ctx_t *ctx, const memcache_item_t *item);

static const img_loader_impl_t coverloader_impl = {
        .memcache_get = coverloader_memcache_get,
        .memcache_put = coverloader_memcache_put,
        .filecache_get = coverloader_filecache_get,
        .filecache_put = coverloader_filecache_put,
        .fetch = coverloader_fetch,
        .run_on_main = coverloader_run_on_main,
};

static const img_loader_cb_t coverloader_cb = {
        .start_cb = img_loader_start_cb,
        .complete_cb = img_loader_result_cb,
        .fail_cb = img_loader_result_cb,
        .cancel_cb = img_loader_cancel_cb,
};

struct coverloader_t {
    img_loader_t *base_loader;
    lv_lru_t *mem_cache;
    lazy_t client;
    coverloader_req_t *reqlist;
    refcounter_t refcounter;
};

typedef struct subimage_info_t {
    int w, h;
    SDL_Rect rect;
} subimage_info_t;

coverloader_t *coverloader_new(app_t *app) {
    coverloader_t *loader = malloc(sizeof(coverloader_t));
    refcounter_init(&loader->refcounter);
    loader->mem_cache = lv_lru_create(1024 * 1024 * 32, 720 * 1024, (lv_lru_free_t *) memcache_item_free, NULL);
    loader->base_loader = img_loader_create(&coverloader_impl, app->backend.executor);
    lazy_init(&loader->client, (lazy_supplier) app_gs_client_new, app);
    loader->reqlist = NULL;
    return loader;
}

void coverloader_unref(coverloader_t *loader) {
    if (!refcounter_unref(&loader->refcounter)) {
        return;
    }
    GS_CLIENT client = lazy_deinit(&loader->client);
    if (client != NULL) {
        gs_destroy(client);
    }
    img_loader_destroy(loader->base_loader);
    lv_lru_del(loader->mem_cache);
    refcounter_destroy(&loader->refcounter);
    free(loader);
}

void coverloader_display(coverloader_t *loader, const uuidstr_t *uuid, int id, lv_obj_t *target,
                         lv_coord_t target_width, lv_coord_t target_height) {
    coverloader_req_t *existing = reqlist_find_by(loader->reqlist, target, reqlist_find_by_target);
    if (existing && existing->task) {
        img_loader_cancel(loader->base_loader, existing->task);
    }

    coverloader_req_t *req = reqlist_new();
    req->loader = loader;
    req->server_id = *uuid;
    req->id = id;
    req->target = target;
    req->target_width = target_width;
    req->target_height = target_height;
    req->finished = false;
    lv_obj_add_event_cb(target, target_deleted_cb, LV_EVENT_DELETE, req);
    loader->reqlist = reqlist_append(loader->reqlist, req);
    refcounter_ref(&loader->refcounter);
    img_loader_task_t *task = img_loader_load(loader->base_loader, req, &coverloader_cb);
    /* If no task returned, then the request has been freed already */
    if (!task) { return; }
    req->task = task;
}

static char *coverloader_cache_dir() {
    return path_cache();
}

static GS_CLIENT coverloader_gs_client(coverloader_t *loader) {
    return lazy_obtain(&loader->client);
}

static void coverloader_cache_item_path(char path[4096], const coverloader_req_t *req) {
    char *cachedir = coverloader_cache_dir();
    char basename[128];
    SDL_snprintf(basename, 128, "%s_%d", (char *) &req->server_id, req->id);
    path_join_to(path, 4096, cachedir, basename);
    free(cachedir);
}

static bool coverloader_memcache_get(coverloader_req_t *req) {
    // Uses result cache instead
    memcache_item_t *result = NULL;
    memcache_key_t key;
    memset(&key, 0, sizeof(key));
    key.id = req->id;
    key.target_width = req->target_width;
    key.target_height = req->target_height;

    (void) key.target_width;
    (void) key.target_height;

    lv_lru_get(req->loader->mem_cache, &key, sizeof(key), (void **) &result);
    req->src = result;

    return result != NULL;
}

static void coverloader_memcache_put(coverloader_req_t *req) {
    SDL_Surface *cached = req->cached;
    if (cached == NULL) {
        return;
    }
    memcache_item_t *result = NULL;
    memcache_key_t key = {
            .id = req->id,
            .target_width = req->target_width,
            .target_height = req->target_height
    };
    lv_lru_get(req->loader->mem_cache, &key, sizeof(key), (void **) &result);
    if (result == NULL) {
        lv_draw_sdl_drv_param_t *param = lv_disp_get_default()->driver->user_data;
        SDL_Renderer *renderer = param->renderer;

        result = memcache_item_new();
        result->target_width = req->target_width;
        result->target_height = req->target_height;
        result->target_radius = lv_obj_get_style_radius(req->target, 0);
        lv_img_dsc_t *src = &result->src;
        lv_sdl_img_data_t *data = &result->data;
        data->type = LV_SDL_IMG_TYPE_TEXTURE;
        src->header.cf = LV_IMG_CF_TRUE_COLOR;
        if (SDL_ISPIXELFORMAT_ALPHA(cached->format->format)) {
            src->header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA;
        }
        int value_length = cached->w * cached->h;
        if (cached->userdata) {
            subimage_info_t *info = cached->userdata;
            data->rect = info->rect;
            value_length = info->w * info->h;
            src->header.w = info->w;
            src->header.h = info->h;
        } else {
            src->header.w = cached->w;
            src->header.h = cached->h;
        }
        data->data.texture = SDL_CreateTextureFromSurface(renderer, cached);
        src->data_size = sizeof(lv_sdl_img_data_t);
        src->data = (const uint8_t *) data;
        lv_lru_set(req->loader->mem_cache, &key, sizeof(key), result, value_length);
    }
    req->src = result;
    req->finished = true;
    if (cached->userdata) {
        SDL_free(cached->userdata);
    }
    SDL_FreeSurface(cached);
}

static bool coverloader_filecache_get(coverloader_req_t *req) {
#if !DEBUG
    SDL_version ver;
    SDL_GetVersion(&ver);
    if (SDL_VERSIONNUM(ver.major, ver.minor, ver.patch) <= SDL_VERSIONNUM(2, 0, 1)) {
        return false;
    }
#endif
    char path[4096];
    coverloader_cache_item_path(path, req);
    SDL_Surface *decoded = IMG_Load(path);
    if (!decoded) { return false; }
    if (cover_is_placeholder(decoded)) {
        SDL_FreeSurface(decoded);
        return false;
    }
    int sw = decoded->w, sh = decoded->h;
    while (sw > req->target_width * 1.5 || sh > req->target_height * 1.5) {
        sw /= 2;
        sh /= 2;
    }
    if (!sw || !sh) {
        // Image is too small to display
        SDL_FreeSurface(decoded);
        return false;
    }
    double srcratio = sw / (double) sh, dstratio = req->target_width / (double) req->target_height;
    SDL_Rect srcrect;
    if (srcratio > dstratio) {
        // Source is wider than destination
        srcrect.h = sh;
        srcrect.w = (int) (sh * dstratio);
        srcrect.y = 0;
        srcrect.x = (sw - srcrect.w) / 2;
    } else {
        // Destination is wider than source
        srcrect.w = sw;
        srcrect.h = (int) (sw / dstratio);
        srcrect.x = 0;
        srcrect.y = (sh - srcrect.h) / 2;
    }
    subimage_info_t *info = SDL_malloc(sizeof(subimage_info_t));
    info->w = req->target_width;
    info->h = req->target_height;
    info->rect = srcrect;

    if (sw == decoded->w && sh == decoded->h) {
        decoded->userdata = info;
        req->cached = decoded;
        return true;
    }

    const SDL_PixelFormat *format = decoded->format;
    SDL_Surface *scaled = SDL_CreateRGBSurface(0, sw, sh, format->BitsPerPixel,
                                               format->Rmask, format->Gmask, format->Bmask, format->Amask);
    SDL_BlitScaled(decoded, NULL, scaled, NULL);
    SDL_FreeSurface(decoded);
    scaled->userdata = info;
    req->cached = scaled;
    return true;
}

static bool cover_is_placeholder(const SDL_Surface *surface) {
    return (surface->w == 130 && surface->h == 180) || (surface->w == 628 && surface->h == 888);
}

static void coverloader_filecache_put(coverloader_req_t *req) {
    // This was done in fetch step
    (void) req;
}

static bool coverloader_fetch(coverloader_req_t *req) {
#if !DEBUG
    SDL_version ver;
    SDL_GetVersion(&ver);
    if (SDL_VERSIONNUM(ver.major, ver.minor, ver.patch) <= SDL_VERSIONNUM(2, 0, 1)) {
        return NULL;
    }
#endif
    const pclist_t *node = pcmanager_node(pcmanager, &req->server_id);
    if (!node) {
        return false;
    }
    char path[4096];
    coverloader_cache_item_path(path, req);
    GS_CLIENT client = coverloader_gs_client(req->loader);
    if (gs_download_cover(client, node->server, req->id, path) != GS_OK) {
        return false;
    }
    return coverloader_filecache_get(req);
}

static void coverloader_run_on_main(img_loader_t *loader, img_loader_run_on_main_fn fn, void *args) {
    (void) loader;
    app_bus_post(global, (bus_actionfunc) fn, args);
}

static void img_loader_start_cb(coverloader_req_t *req) {
    appitem_viewholder_t *holder = req->target->user_data;
    img_set_cover(req->target, NULL);
    lv_obj_add_flag(holder->title, LV_OBJ_FLAG_HIDDEN);
}

static void img_loader_result_cb(coverloader_req_t *req) {
    req->task = NULL;
    coverloader_t *loader = req->loader;
    if (req->target == NULL) {
        goto done;
    }

    lv_obj_remove_event_cb(req->target, target_deleted_cb);
    appitem_viewholder_t *holder = req->target->user_data;
    img_set_cover(req->target, req->src);
    if (req->src) {
        lv_obj_add_flag(holder->title, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_flag(holder->title, LV_OBJ_FLAG_HIDDEN);
    }
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

static void img_set_cover(lv_obj_t *obj, memcache_item_t *src) {
    appitem_viewholder_t *holder = lv_obj_get_user_data(obj);

    /* If the old src is not default cover, assume it's memcache item and unref this obj from it */
    const void *old_src = lv_img_get_src(obj);
    bool skip_add_del_cb = false;
    if (old_src != NULL && lv_img_src_get_type(old_src) == LV_IMG_SRC_VARIABLE) {
        const memcache_item_t *old_img = old_src;
        /* for memcache item, src.data is pointer to data */
        if (old_img->src.data == (const void *) &old_img->data && old_img->src.data_size == sizeof(lv_sdl_img_data_t)) {
            memcache_item_unref_obj((memcache_item_t *) old_img, obj);
            skip_add_del_cb = true;
        }
    }

    if (src != NULL) {
        lv_img_set_src(obj, src);
        memcache_item_ref_obj(src, obj);
        // If the object is deleted, mark the src orphaned. So it will not access obj when recycled.
        if (!skip_add_del_cb) {
            lv_obj_add_event_cb(obj, target_src_unlink_cb, LV_EVENT_DELETE, NULL);
        }
    } else {
        lv_img_set_src(obj, &holder->styles->defcover_src);
        lv_obj_remove_event_cb(obj, target_src_unlink_cb);
    }
}

struct memcache_item_t *memcache_item_new() {
    memcache_item_t *item = calloc(1, sizeof(memcache_item_t));
    _lv_ll_init(&item->objs, sizeof(lv_obj_t *));
    return item;
}

static void memcache_item_free(memcache_item_t *item) {
    lv_disp_drv_t *driver = lv_disp_get_default()->driver;
    lv_draw_sdl_ctx_t *ctx = (lv_draw_sdl_ctx_t *) driver->draw_ctx;

    /* Make sure the obj isn't referencing the src anymore */
    lv_obj_t **i;
    _LV_LL_READ(&item->objs, i) {
        lv_obj_t *obj = *i;
        if (obj == NULL) { continue; }
        appitem_viewholder_t *holder = lv_obj_get_user_data(obj);
        lv_img_set_src(obj, &holder->styles->defcover_src);
        lv_obj_remove_event_cb(obj, target_src_unlink_cb);
        lv_obj_clear_flag(holder->title, LV_OBJ_FLAG_HIDDEN);
    }
    SDL_DestroyTexture(item->data.data.texture);

    // Purge internal texture cache too
    purge_img_cache(ctx, item);
    purge_corners_cache(ctx, item);

    /* unref all objs to this item */
    _lv_ll_clear(&item->objs);
    free(item);
}

static void purge_img_cache(lv_draw_sdl_ctx_t *ctx, const memcache_item_t *item) {
    struct __attribute__ ((__packed__)) {
        lv_draw_sdl_cache_key_head_img_t header;
        const void *src;
    } key_to_remove;
    memset(&key_to_remove, 0, sizeof(key_to_remove));
    key_to_remove.header.magic = LV_GPU_CACHE_KEY_MAGIC_IMG;
    key_to_remove.header.type = LV_IMG_SRC_VARIABLE;
    key_to_remove.header.frame_id = 0;
    key_to_remove.src = &item->src;

    lv_lru_remove(ctx->internals->texture_cache, &key_to_remove, sizeof(key_to_remove));
}

static void purge_corners_cache(lv_draw_sdl_ctx_t *ctx, const memcache_item_t *item) {
    struct {
        lv_sdl_cache_key_magic_t magic;
        const SDL_Texture *texture;
        lv_coord_t w, h, radius;
    } key_to_remove;

    memset(&key_to_remove, 0, sizeof(key_to_remove));
    key_to_remove.magic = LV_GPU_CACHE_KEY_MAGIC_IMG_ROUNDED_CORNERS;
    key_to_remove.texture = item->data.data.texture;
    key_to_remove.w = item->target_width;
    key_to_remove.h = item->target_height;
    key_to_remove.radius = LV_MIN3(item->target_radius, item->target_width, item->target_height);

    lv_lru_remove(ctx->internals->texture_cache, &key_to_remove, sizeof(key_to_remove));
}

static void memcache_item_ref_obj(memcache_item_t *item, lv_obj_t *obj) {
    lv_obj_t **node = _lv_ll_ins_tail(&item->objs);
    *node = obj;
}

static void memcache_item_unref_obj(memcache_item_t *item, const lv_obj_t *obj) {
    lv_obj_t **i;
    _LV_LL_READ_BACK(&item->objs, i) {
        if (obj == *i) {
            break;
        }
    }
    if (i == NULL) { return; }
    _lv_ll_remove(&item->objs, i);
    lv_mem_free(i);
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

static void target_src_unlink_cb(lv_event_t *e) {
    lv_obj_t *obj = lv_event_get_target(e);
    memcache_item_unref_obj((memcache_item_t *) lv_img_get_src(obj), obj);
}