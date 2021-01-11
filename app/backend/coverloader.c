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

#include "util/bus.h"
#include "util/user_event.h"
#include "util/lruc.h"
#include "util/path.h"
#include "util/gs/clientex.h"

#include "libgamestream/errors.h"

#include <SDL.h>
#include <SDL_image.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_opengles2.h>

enum IMAGE_STATE_T
{
    IMAGE_STATE_QUEUED,
    IMAGE_STATE_LOADING,
    IMAGE_STATE_FINISHED
};

struct CACHE_ITEM_T
{
    int id;
    PSERVER_DATA server;
    enum IMAGE_STATE_T state;
    struct nk_image *data;
};

static pthread_t coverloader_worker_thread;
static lruc *coverloader_mem_cache;
static struct CACHE_ITEM_T *coverloader_current_task;
static bool coverloader_working_running;

WORKER_THREAD static void *coverloader_worker(void *);
WORKER_THREAD static bool coverloader_decode_image(int id, struct nk_image *decoded);
WORKER_THREAD static bool coverloader_fetch_image(PSERVER_DATA server, int id);
MAIN_THREAD static void coverloader_load(struct CACHE_ITEM_T *req);
MAIN_THREAD static struct CACHE_ITEM_T *cache_item_new(PSERVER_DATA server, int id);
THREAD_SAFE static void coverloader_notify_change(struct CACHE_ITEM_T *req, enum IMAGE_STATE_T state, struct nk_image *data);
THREAD_SAFE static void coverloader_notify_decoded(struct CACHE_ITEM_T *req, struct nk_image *decoded);
THREAD_SAFE static void coverloader_cache_item_free(void *p);

static char *coverloader_cache_dir();

MAIN_THREAD
void coverloader_init()
{
    coverloader_mem_cache = lruc_new(128, 4, &coverloader_cache_item_free);
    coverloader_current_task = NULL;
    coverloader_working_running = true;
    pthread_create(&coverloader_worker_thread, NULL, coverloader_worker, NULL);
}

MAIN_THREAD
void coverloader_destroy()
{
    coverloader_working_running = false;
    pthread_join(coverloader_worker_thread, NULL);
    lruc_free(coverloader_mem_cache);
}

MAIN_THREAD
struct nk_image *coverloader_get(PSERVER_DATA server, int id)
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
        cached = cache_item_new(server, id);
        lruc_set(coverloader_mem_cache, id, cached, 4);
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
        if (!req)
        {
            continue;
        }
        // Don't do anything if it's not in initial state
        if (req->state != IMAGE_STATE_QUEUED)
        {
            continue;
        }
        coverloader_notify_change(req, IMAGE_STATE_LOADING, NULL);
        struct nk_image decoded = nk_image_id(0);
        // Load from local file cache
        if (coverloader_decode_image(req->id, &decoded))
        {
            coverloader_notify_decoded(req, &decoded);
            continue;
        }
        // Local cache miss, fetch via API
        if (!coverloader_fetch_image(req->server, req->id))
        {
            // Unable to fetch, store negative result
            coverloader_notify_change(req, IMAGE_STATE_FINISHED, NULL);
            continue;
        }
        // Now the file is in local cache, load again
        if (coverloader_decode_image(req->id, &decoded))
        {
            coverloader_notify_decoded(req, &decoded);
            continue;
        }
        coverloader_notify_change(req, IMAGE_STATE_FINISHED, NULL);
    }
    pthread_exit(NULL);
}

MAIN_THREAD
struct CACHE_ITEM_T *cache_item_new(PSERVER_DATA server, int id)
{
    struct CACHE_ITEM_T *item = malloc(sizeof(struct CACHE_ITEM_T));
    item->id = id;
    item->server = server;
    item->state = IMAGE_STATE_QUEUED;
    item->data = NULL;
    return item;
}

MAIN_THREAD
void coverloader_load(struct CACHE_ITEM_T *req)
{
    // This will make worker start to do the loading work
    coverloader_current_task = req;
}

// Send state change event to a thread safe message queue
THREAD_SAFE
void coverloader_notify_change(struct CACHE_ITEM_T *req, enum IMAGE_STATE_T state, struct nk_image *data)
{
    struct CACHE_ITEM_T *update = malloc(sizeof(struct CACHE_ITEM_T));
    update->id = req->id;
    update->server = req->server;
    update->state = state;
    update->data = data;
    if (!bus_pushevent(USER_IL_STATE_CHANGED, req, update))
    {
        free(update);
    }
}

THREAD_SAFE
void coverloader_notify_decoded(struct CACHE_ITEM_T *req, struct nk_image *decoded)
{
    // Don't free this value
    void *deccpy = malloc(sizeof(struct nk_image));
    memcpy(deccpy, decoded, sizeof(struct nk_image));
    bus_pushevent(USER_IL_IMAGE_DECODED, req, deccpy);
}

WORKER_THREAD
bool coverloader_decode_image(int id, struct nk_image *decoded)
{
    char *cachedir = coverloader_cache_dir();
    char path[4096];
    sprintf(path, "%s/%d", cachedir, id);
    free(cachedir);
    SDL_Surface *s = IMG_Load(path);
    if (!s)
    {
        return false;
    }
    decoded->w = s->w;
    decoded->h = s->h;
    decoded->region[0] = 0;
    decoded->region[1] = 0;
    decoded->region[2] = s->w;
    decoded->region[3] = s->h;
    decoded->handle.ptr = s;
    return true;
}

WORKER_THREAD
bool coverloader_fetch_image(PSERVER_DATA server, int id)
{
    char *cachedir = coverloader_cache_dir();
    char path[4096];
    sprintf(path, "%s/%d", cachedir, id);
    free(cachedir);
    return gs_download_cover(server, id, path) == GS_OK;
}

static GLuint gen_texture_from_sdl(SDL_Surface *surface)
{
    GLuint texture;
    GLenum texture_format;
    GLint nOfColors;
    // get the number of channels in the SDL surface
    nOfColors = surface->format->BytesPerPixel;
    if (nOfColors == 4) // contains an alpha channel
    {
        if (surface->format->Rmask == 0x000000ff)
            texture_format = GL_RGBA;
        else
            texture_format = GL_BGRA;
    }
    else if (nOfColors == 3) // no alpha channel
    {
        if (surface->format->Rmask == 0x000000ff)
            texture_format = GL_RGB;
        else
            texture_format = GL_BGR;
    }
    else
    {
        printf("warning: the image is not truecolor..  this will probably break\n");
        // this error should not go unhandled
    }

    // Have OpenGL generate a texture object handle for us
    glGenTextures(1, &texture);

    // Bind the texture object
    glBindTexture(GL_TEXTURE_2D, texture);

    // Set the texture's stretching properties
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Edit the texture object's image data using the information SDL_Surface gives us
    glTexImage2D(GL_TEXTURE_2D, 0, texture_format, surface->w, surface->h, 0,
                 texture_format, GL_UNSIGNED_BYTE, surface->pixels);
    return texture;
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
        return true;
    }
    else if (which == USER_IL_IMAGE_DECODED)
    {
        struct CACHE_ITEM_T *req = data1;
        struct nk_image *img = data2;

        SDL_Surface *surface = img->handle.ptr;

        img->handle.id = gen_texture_from_sdl(surface);

        // Surface has been used up
        SDL_FreeSurface(surface);
        req->data = img;
        req->state = IMAGE_STATE_FINISHED;
        coverloader_current_task = NULL;
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
        glDeleteTextures(1, &(item->data->handle.id));
    }
    free(item);
}