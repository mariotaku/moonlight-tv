//
// Created by Mariotaku on 2021/09/03.
//

#include "img_loader.h"
#include <SDL.h>
#include <stdbool.h>

struct img_loader_task_t {
    void *request;
    img_loader_cb_t cb;
    int cancelled;
    struct img_loader_task_t *next;
};

struct img_loader_t {
    img_loader_impl_t impl;
    struct img_loader_task_t *task_queue;
    SDL_mutex *queue_lock;
    SDL_cond *queue_cond;
    SDL_Thread *worker_thread;
    bool destroyed;
};

typedef struct notify_cb_t {
    img_loader_fn2 fn;
    void *arg1;
    void *arg2;
    bool notified;
    SDL_mutex *mutex;
    SDL_cond *cond;
    const bool *destroyed;
} notify_cb_t;

#define LINKEDLIST_IMPL
#define LINKEDLIST_TYPE struct img_loader_task_t
#define LINKEDLIST_PREFIX tasklist
#define LINKEDLIST_DOUBLE 0

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
    loader->destroyed = false;
    loader->worker_thread = SDL_CreateThread((SDL_ThreadFunction) loader_worker, "img_loader", loader);
    return loader;
}

void img_loader_destroy(img_loader_t *loader) {
    SDL_assert(!loader->destroyed);
    SDL_Thread *thread = loader->worker_thread;
    SDL_LockMutex(loader->queue_lock);
    loader->destroyed = true;
    SDL_CondSignal(loader->queue_cond);
    SDL_UnlockMutex(loader->queue_lock);
    SDL_DetachThread(thread);
}

img_loader_task_t *img_loader_load(img_loader_t *loader, void *request, const img_loader_cb_t *cb) {
    SDL_assert(!loader->destroyed);
    cb->start_cb(request);
    void *cached = NULL;
    // Memory cache found, finish loading
    if (loader->impl.memcache_get(request, &cached)) {
        cb->complete_cb(request, cached);
        return NULL;
    }
    SDL_LockMutex(loader->queue_lock);
    img_loader_task_t *task = tasklist_new();
    task->request = request;
    task->cancelled = SDL_FALSE;
    SDL_memcpy(&task->cb, cb, sizeof(img_loader_cb_t));
    loader_task_push(loader, task);
    SDL_CondSignal(loader->queue_cond);
    SDL_UnlockMutex(loader->queue_lock);
    return task;
}

void img_loader_cancel(img_loader_t *loader, img_loader_task_t *task) {
    SDL_assert(!loader->destroyed);
    SDL_LockMutex(loader->queue_lock);
    if (task) {
        task->cancelled = SDL_TRUE;
    }
    SDL_UnlockMutex(loader->queue_lock);
}

static int loader_worker(img_loader_t *loader) {
    while (!loader->destroyed) {
        img_loader_task_t *task = loader_task_poll(loader);
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
    while (loader->task_queue == NULL) {
        SDL_CondWait(loader->queue_cond, loader->queue_lock);
    }
    img_loader_task_t *task = loader->task_queue;
    SDL_assert(task);
    loader->task_queue = task->next;
    SDL_UnlockMutex(loader->queue_lock);
    return task;
}

static void loader_task_execute(img_loader_t *loader, img_loader_task_t *task) {
    void *request = task->request;
    void *cached = NULL;
    if (!loader->impl.filecache_get(request, &cached)) {
        if (!loader->impl.fetch(request, &cached)) {
            // call fail_cb
            img_loader_fn cb = task->cancelled ? task->cb.cancel_cb : task->cb.fail_cb;
            run_on_main(loader, (img_loader_fn2) cb, request, NULL);
            return;
        }
        if (loader->destroyed) return;
        loader->impl.filecache_put(request, cached);
    }
    if (loader->destroyed) return;
    run_on_main(loader, loader->impl.memcache_put, request, cached);
    if (loader->destroyed) return;
    img_loader_fn2 cb = task->cancelled ? (img_loader_fn2) task->cb.cancel_cb : task->cb.complete_cb;
    run_on_main(loader, cb, request, cached);
}

static void run_on_main(img_loader_t *loader, img_loader_fn2 fn, void *arg1, void *arg2) {
    if (loader->destroyed) return;
    notify_cb_t args = {
            .arg1 = arg1, .arg2 = arg2, .fn = fn,
            .mutex = SDL_CreateMutex(), .cond = SDL_CreateCond(),
            .destroyed = &loader->destroyed
    };
    loader->impl.run_on_main(loader, (img_loader_fn) notify_cb, &args);
    SDL_LockMutex(args.mutex);
    while (!args.notified) {
        SDL_CondWait(args.cond, args.mutex);
    }
    SDL_UnlockMutex(args.mutex);
    SDL_DestroyMutex(args.mutex);
    SDL_DestroyCond(args.cond);
}

static void notify_cb(notify_cb_t *args) {
    SDL_LockMutex(args->mutex);
    if (!*args->destroyed) {
        args->fn(args->arg1, args->arg2);
    }
    args->notified = true;
    SDL_CondSignal(args->cond);
    SDL_UnlockMutex(args->mutex);
}

static void task_destroy(img_loader_task_t *task) {
    free(task);
}