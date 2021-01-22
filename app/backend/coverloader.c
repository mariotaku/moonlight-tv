#define _COVERLOADER_IMPL

#include "coverloader.h"

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include <sys/stat.h>

#include <uuid/uuid.h>

#include "nuklear/ext_image.h"

#include "util/bus.h"
#include "util/user_event.h"
#include "util/lruc.h"
#include "util/path.h"
#include "util/gs/clientex.h"

#include "libgamestream/errors.h"

enum IMAGE_STATE_T
{
    IMAGE_STATE_QUEUED,
    IMAGE_STATE_LOADING,
    IMAGE_STATE_FINISHED
};

struct CACHE_ITEM_T
{
    int id;
    PSERVER_LIST node;
    enum IMAGE_STATE_T state;
    int width, height;
    struct nk_image *data;
};

static pthread_t coverloader_worker_thread;
static pthread_mutex_t coverloader_state_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t coverloader_queue_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t coverloader_queue_cond = PTHREAD_COND_INITIALIZER;
static lruc *coverloader_mem_cache;
static struct CACHE_ITEM_T *coverloader_current_task;
static bool coverloader_working_running;

WORKER_THREAD static void *coverloader_worker(void *);
WORKER_THREAD static bool coverloader_decode_image(struct CACHE_ITEM_T *req, struct nk_image *decoded);
WORKER_THREAD static bool coverloader_fetch_image(PSERVER_LIST node, int id);
MAIN_THREAD static void coverloader_load(struct CACHE_ITEM_T *req);
MAIN_THREAD static struct CACHE_ITEM_T *cache_item_new(PSERVER_LIST node, int id);
WORKER_THREAD static void coverloader_notify_change(struct CACHE_ITEM_T *req, enum IMAGE_STATE_T state, struct nk_image *data);
WORKER_THREAD static void coverloader_notify_decoded(struct CACHE_ITEM_T *req, struct nk_image *decoded);
THREAD_SAFE static void coverloader_cache_item_free(void *p);

static char *coverloader_cache_dir();
static bool _cover_is_placeholder(struct nk_image *bmp);
static bool _cover_update_region(struct nk_image *img, int width, int height);

#define ITEM_AVERAGE_SIZE 360000

MAIN_THREAD
void coverloader_init()
{
    // 32MB limit, 720K each
    coverloader_mem_cache = lruc_new(32 * 1024 * 1024, ITEM_AVERAGE_SIZE, &coverloader_cache_item_free);
    coverloader_current_task = NULL;
    coverloader_working_running = true;

    pthread_create(&coverloader_worker_thread, NULL, coverloader_worker, NULL);
#if OS_LINUX
    pthread_setname_np(coverloader_worker_thread, "coverloader");
#endif
}

MAIN_THREAD
void coverloader_destroy()
{
    coverloader_working_running = false;
    pthread_cond_signal(&coverloader_queue_cond);
    pthread_join(coverloader_worker_thread, NULL);
    pthread_mutex_destroy(&coverloader_state_lock);
    lruc_free(coverloader_mem_cache);
}

MAIN_THREAD
struct nk_image *coverloader_get(PSERVER_LIST node, int id, int target_width, int target_height)
{
    // First find in memory cache
    struct CACHE_ITEM_T *cached = NULL;
    lruc_get(coverloader_mem_cache, id, (void **)(&cached));
    if (cached && cached->state == IMAGE_STATE_FINISHED)
    {
        return cached->data;
    }
    // Memory cache miss, start image loading pipeline.
    if (!cached)
    {
        cached = cache_item_new(node, id);
        cached->width = target_width;
        cached->height = target_height;
        lruc_set(coverloader_mem_cache, id, cached, ITEM_AVERAGE_SIZE);
    }
    // Here we have single task for image loading.
    if (cached->state == IMAGE_STATE_QUEUED && !coverloader_current_task)
    {
        coverloader_load(cached);
    }
    return NULL;
}

WORKER_THREAD
void *coverloader_worker(void *unused)
{
    while (coverloader_working_running)
    {
        struct CACHE_ITEM_T *req = coverloader_current_task;
        // Don't do anything if it's not in initial state
        if (!req || req->state != IMAGE_STATE_QUEUED)
        {
            pthread_mutex_lock(&coverloader_queue_lock);
            pthread_cond_wait(&coverloader_queue_cond, &coverloader_queue_lock);
            pthread_mutex_unlock(&coverloader_queue_lock);
            continue;
        }
        coverloader_notify_change(req, IMAGE_STATE_LOADING, NULL);
        struct nk_image decoded = nk_image_id(0);
        // Load from local file cache
        if (coverloader_decode_image(req, &decoded))
        {
            printf("Local file cache hit for %d\n", req->id);
            coverloader_notify_decoded(req, &decoded);
            continue;
        }
        // Local cache miss, fetch via API
        if (!coverloader_fetch_image(req->node, req->id))
        {
            // Unable to fetch, store negative result
            coverloader_notify_change(req, IMAGE_STATE_FINISHED, NULL);
            continue;
        }
        printf("Cover fetched for %d\n", req->id);
        // Now the file is in local cache, load again
        if (coverloader_decode_image(req, &decoded))
        {
            printf("Downloaded cover decoded for %d\n", req->id);
            coverloader_notify_decoded(req, &decoded);
            continue;
        }
        coverloader_notify_change(req, IMAGE_STATE_FINISHED, NULL);
    }
    return NULL;
}

MAIN_THREAD
struct CACHE_ITEM_T *cache_item_new(PSERVER_LIST node, int id)
{
    struct CACHE_ITEM_T *item = malloc(sizeof(struct CACHE_ITEM_T));
    item->id = id;
    item->node = node;
    item->state = IMAGE_STATE_QUEUED;
    item->data = NULL;
    return item;
}

MAIN_THREAD
void coverloader_load(struct CACHE_ITEM_T *req)
{
    // This will make worker start to do the loading work
    coverloader_current_task = req;
    pthread_cond_signal(&coverloader_queue_cond);
}

// Send state change event to a thread safe message queue
WORKER_THREAD
void coverloader_notify_change(struct CACHE_ITEM_T *req, enum IMAGE_STATE_T state, struct nk_image *data)
{
    pthread_mutex_lock(&coverloader_state_lock);
    struct CACHE_ITEM_T *update = malloc(sizeof(struct CACHE_ITEM_T));
    update->id = req->id;
    update->node = req->node;
    update->state = state;
    update->data = data;
    if (!bus_pushevent(USER_IL_STATE_CHANGED, req, update))
    {
        free(update);
    }
}

WORKER_THREAD
void coverloader_notify_decoded(struct CACHE_ITEM_T *req, struct nk_image *decoded)
{
    pthread_mutex_lock(&coverloader_state_lock);
    // Don't free this value
    void *deccpy = malloc(sizeof(struct nk_image));
    memcpy(deccpy, decoded, sizeof(struct nk_image));
    bus_pushevent(USER_IL_IMAGE_DECODED, req, deccpy);
}

WORKER_THREAD
bool coverloader_decode_image(struct CACHE_ITEM_T *req, struct nk_image *decoded)
{
    char *cachedir = coverloader_cache_dir();
    char path[4096];
    sprintf(path, "%s/%d", cachedir, req->id);
    free(cachedir);
    if (!nk_imageloadf(path, decoded))
    {
        return false;
    }
    if (_cover_is_placeholder(decoded))
    {
        nk_imagebmpfree(decoded);
        memset(decoded, 0, sizeof(struct nk_image));
    }
    else
    {
        _cover_update_region(decoded, req->width, req->height);
    }
    return true;
}

WORKER_THREAD
bool coverloader_fetch_image(PSERVER_LIST node, int id)
{
    char *cachedir = coverloader_cache_dir();
    char path[4096];
    sprintf(path, "%s/%d", cachedir, id);
    free(cachedir);
    return gs_download_cover(node->server, id, path) == GS_OK;
}

// We shouldn't write any internal state outside main thread. So do it here
MAIN_THREAD
bool coverloader_dispatch_userevent(int which, void *data1, void *data2)
{
    if (which == USER_IL_STATE_CHANGED)
    {
        struct CACHE_ITEM_T *req = data1, *update = data2;
        req->state = update->state;
        req->data = update->data;
        if (req->state == IMAGE_STATE_FINISHED)
        {
            coverloader_current_task = NULL;
        }
        free(data2);
        pthread_mutex_unlock(&coverloader_state_lock);
        return true;
    }
    else if (which == USER_IL_IMAGE_DECODED)
    {
        struct CACHE_ITEM_T *req = data1;
        struct nk_image *img = data2;
        unsigned int img_size;
        if (img->w > 0 && img->h > 0)
        {
            size_t bpp = nk_imagebmppxsize(img);
            nk_image2texture(img);
            img_size = img->w * img->h * bpp;
        }
        else
        {
            // Placeholder image
            img_size = 1;
        }

        struct CACHE_ITEM_T *cached = malloc(sizeof(struct CACHE_ITEM_T));
        memcpy(cached, req, sizeof(struct CACHE_ITEM_T));
        cached->data = img;
        cached->state = IMAGE_STATE_FINISHED;

        lruc_set(coverloader_mem_cache, cached->id, cached, img_size);

        coverloader_current_task = NULL;
        pthread_mutex_unlock(&coverloader_state_lock);
        return true;
    }
    else
    {
        return false;
    }
}

char *coverloader_cache_dir()
{
    char *cachedir = getenv("XDG_CACHE_DIR"), *confdir = NULL;
    if (cachedir)
    {
        confdir = path_join(cachedir, "moonlight-tv");
    }
    else
    {
        cachedir = getenv("HOME");
        confdir = path_join(cachedir, ".moonlight-tv-covers");
    }

    if (access(confdir, F_OK) == -1)
    {
        if (errno == ENOENT)
        {
            mkdir(confdir, 0755);
        }
    }
    return confdir;
}

void coverloader_cache_item_free(void *p)
{
    struct CACHE_ITEM_T *item = p;
    if (item->data)
    {
        nk_imagetexturefree(item->data);
    }
    free(item);
}

bool _cover_is_placeholder(struct nk_image *bmp)
{
    return (bmp->w == 130 && bmp->h == 180) || (bmp->w == 628 && bmp->h == 888);
}

bool _cover_update_region(struct nk_image *img, int width, int height)
{
    if (width > height)
    {
        // Don't care about this case
        return false;
    }
    float sratio = img->w / (float)img->h, dratio = width / (float)height;
    if (dratio > sratio)
    {
        // dst image is wider, adjust src vertical crop to fit dst width
        int croph = img->w / dratio;
        // horizontal as-is
        img->region[0] = 0;
        img->region[1] = (img->h - croph) / 2;

        img->region[2] = img->w;
        img->region[3] = croph;
    }
    else
    {
        // src image is wider, adjust src horizontal crop fit dst height
        int cropw = img->h * dratio;
        img->region[0] = (img->w - cropw) / 2;
        // vertical as-is
        img->region[1] = 0;

        img->region[2] = cropw;
        img->region[3] = img->h;
    }
    return true;
}