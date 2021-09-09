//
// Created by Mariotaku on 2021/09/03.
//

#include "img_loader.h"
#include <SDL.h>

struct img_loader_task_t {
    void *request;
    img_loader_cb_t cb;
    int cancelled;
    struct img_loader_task_t *prev;
    struct img_loader_task_t *next;
};

struct img_loader_t {
    img_loader_impl_t impl;
    struct img_loader_task_t *task_queue;
    SDL_mutex *queue_lock;
    SDL_cond *queue_cond;
    SDL_Thread *worker_thread;
    int destroyed;
};

typedef struct notify_cb_t {
    img_loader_fn2 fn;
    void *arg1;
    void *arg2;
} notify_cb_t;

#define LINKEDLIST_IMPL
#define LINKEDLIST_TYPE struct img_loader_task_t
#define LINKEDLIST_PREFIX tasklist
#define LINKEDLIST_DOUBLE 1

#include "util/linked_list.h"

#undef LINKEDLIST_TYPE
#undef LINKEDLIST_PREFIX
#undef LINKEDLIST_DOUBLE

static void loader_task_execute(img_loader_t *loader, img_loader_task_t *task);

static void loader_task_push(img_loader_t *loader, img_loader_task_t *task);

static img_loader_task_t *loader_task_poll(img_loader_t *loader);

static int loader_worker(img_loader_t *loader);

static void notify_cb(notify_cb_t *args);

static void run_on_main(img_loader_t *loader, img_loader_fn2 fn, void *arg1, void *arg2);

static int tasklist_find_id(struct img_loader_task_t *p, const void *fv);

static void task_destroy(img_loader_task_t *);

img_loader_t *img_loader_create(const img_loader_impl_t *impl) {
    img_loader_t *loader = SDL_malloc(sizeof(img_loader_t));
    SDL_memcpy(&loader->impl, impl, sizeof(img_loader_impl_t));
    loader->task_queue = NULL;
    loader->queue_lock = SDL_CreateMutex();
    loader->queue_cond = SDL_CreateCond();
    loader->destroyed = SDL_FALSE;
    loader->worker_thread = SDL_CreateThread((SDL_ThreadFunction) loader_worker, "img_loader", loader);
    return loader;
}

void img_loader_destroy(img_loader_t *loader) {
    loader->destroyed = SDL_TRUE;
    SDL_CondSignal(loader->queue_cond);
}

img_loader_task_t *img_loader_load(img_loader_t *loader, void *request, const img_loader_cb_t *cb) {
    cb->start_cb(request);
    void *cached = loader->impl.memcache_get(request);
    // Memory cache found, finish loading
    if (cached) {
        cb->complete_cb(request, cached);
        return NULL;
    }
    img_loader_task_t *task = tasklist_new();
    task->request = request;
    task->cancelled = SDL_FALSE;
    SDL_memcpy(&task->cb, cb, sizeof(img_loader_cb_t));
    loader_task_push(loader, task);
    SDL_CondSignal(loader->queue_cond);
    return task;
}

void img_loader_cancel(img_loader_t *loader, img_loader_task_t *task) {
    SDL_LockMutex(loader->queue_lock);
    if (task) {
        task->cancelled = SDL_TRUE;
    }
    SDL_UnlockMutex(loader->queue_lock);
}

static int loader_worker(img_loader_t *loader) {
    while (!loader->destroyed) {
        img_loader_task_t *task = loader_task_poll(loader);
        if (!task) {
            SDL_LockMutex(loader->queue_lock);
            SDL_CondWait(loader->queue_cond, loader->queue_lock);
            SDL_UnlockMutex(loader->queue_lock);
            continue;
        }
        loader_task_execute(loader, task);
        free(task);
    }
    SDL_LockMutex(loader->queue_lock);
    tasklist_free(loader->task_queue, task_destroy);
    loader->task_queue = NULL;
    SDL_UnlockMutex(loader->queue_lock);

    SDL_DestroyCond(loader->queue_cond);
    SDL_DestroyMutex(loader->queue_lock);
    loader->queue_cond = NULL;
    loader->queue_lock = NULL;
    SDL_free(loader);
    return 0;
}

static void loader_task_push(img_loader_t *loader, img_loader_task_t *task) {
    loader->task_queue = tasklist_append(loader->task_queue, task);
}

static img_loader_task_t *loader_task_poll(img_loader_t *loader) {
    SDL_LockMutex(loader->queue_lock);
    img_loader_task_t *task = loader->task_queue;
    loader->task_queue = task ? task->next : NULL;
    SDL_UnlockMutex(loader->queue_lock);
    return task;
}

static void loader_task_execute(img_loader_t *loader, img_loader_task_t *task) {
    void *request = task->request;
    void *cached = loader->impl.filecache_get(request);
    if (loader->destroyed) return;
    if (!cached) {
        cached = loader->impl.fetch(request);
        if (loader->destroyed) return;
        if (!cached) {
            // call fail_cb
            img_loader_fn cb = task->cancelled ? task->cb.cancel_cb : task->cb.fail_cb;
            run_on_main(loader, (img_loader_fn2) cb, request, NULL);
            return;
        }
        loader->impl.filecache_put(request, cached);
    }
    if (loader->destroyed) return;
    run_on_main(loader, loader->impl.memcache_put, request, cached);
    if (loader->impl.memcache_put_wait) {
        loader->impl.memcache_put_wait(request);
    }
    if (loader->destroyed) return;
    img_loader_fn2 cb = task->cancelled ? (img_loader_fn2) task->cb.cancel_cb : task->cb.complete_cb;
    run_on_main(loader, cb, request, cached);
}

static void run_on_main(img_loader_t *loader, img_loader_fn2 fn, void *arg1, void *arg2) {
    if (loader->destroyed) return;
    notify_cb_t *args = malloc(sizeof(notify_cb_t));
    args->arg1 = arg1;
    args->arg2 = arg2;
    args->fn = fn;
    loader->impl.run_on_main(loader, (img_loader_fn) notify_cb, args);
}

static void notify_cb(notify_cb_t *args) {
    args->fn(args->arg1, args->arg2);
    free(args);
}

static void task_destroy(img_loader_task_t *task) {
    free(task);
}