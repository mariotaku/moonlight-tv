#include "app.h"
#include "img_loader.h"
#include "executor.h"
#include "bus.h"

#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>

#include <SDL2/SDL.h>

struct img_loader_task_t {
    void *request;
    void *result;
    img_loader_cb_t cb;
    struct img_loader_t *loader;
    const executor_task_t *task;
};

struct img_loader_t {
    img_loader_impl_t impl;
    executor_t *executor;
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

static int task_execute(img_loader_task_t *task);

static bool task_cancelled(img_loader_task_t *task);

static void task_destroy(img_loader_task_t *task, int result);

static void notify_cb(notify_cb_t *args);

static void run_on_main(img_loader_t *loader, img_loader_fn2 fn, void *arg1, void *arg2);

static void img_loader_free(void *arg, int result);

img_loader_t *img_loader_create(const img_loader_impl_t *impl, executor_t *executor) {
    img_loader_t *loader = SDL_calloc(1, sizeof(img_loader_t));
    loader->impl = *impl;
    loader->executor = executor;
    return loader;
}

void img_loader_destroy(img_loader_t *loader) {
    SDL_assert_release(!loader->destroyed);
    loader->destroyed = true;
    executor_submit(loader->executor, executor_noop, img_loader_free, loader);
}

img_loader_task_t *img_loader_load(img_loader_t *loader, void *request, const img_loader_cb_t *cb) {
    SDL_assert_release(!loader->destroyed);
    cb->start_cb(request);
    void *cached = NULL;
    // Memory cache found, finish loading
    if (loader->impl.memcache_get(request, &cached)) {
        cb->complete_cb(request, cached);
        return NULL;
    }
    img_loader_task_t *task = SDL_calloc(1, sizeof(img_loader_task_t));
    task->loader = loader;
    task->request = request;
    task->cb = *cb;
    task->task = executor_submit(loader->executor, (executor_action_cb) task_execute,
                                 (executor_cleanup_cb) task_destroy, task);
    return task;
}

void img_loader_cancel(img_loader_t *loader, img_loader_task_t *task) {
    SDL_assert_release(!loader->destroyed);
    executor_cancel(loader->executor, task->task);
}

static int task_execute(img_loader_task_t *task) {
    img_loader_t *loader = task->loader;
    void *request = task->request;
    if (!loader->impl.filecache_get(request, &task->result)) {
        if (!loader->impl.fetch(request, &task->result)) {
            // call fail_cb
            img_loader_fn cb = task_cancelled(task) ? task->cb.cancel_cb : task->cb.fail_cb;
            run_on_main(loader, (img_loader_fn2) cb, request, NULL);
            return EIO;
        }
        if (loader->destroyed) {
            return ECANCELED;
        }
        loader->impl.filecache_put(request, task->result);
    }
    run_on_main(loader, loader->impl.memcache_put, request, task->result);
    return 0;
}

static void task_destroy(img_loader_task_t *task, int result) {
    img_loader_t *loader = task->loader;
    void *request = task->request;
    if (result == 0) {
        run_on_main(loader, task->cb.complete_cb, request, task->result);
    } else if (result == ECANCELED) {
        run_on_main(loader, (img_loader_fn2) task->cb.cancel_cb, request, NULL);
    } else {
        run_on_main(loader, (img_loader_fn2) task->cb.fail_cb, request, NULL);
    }
    SDL_free(task);
}

static bool task_cancelled(img_loader_task_t *task) {
    return executor_task_state(task->loader->executor, task->task);
}

static void run_on_main(img_loader_t *loader, img_loader_fn2 fn, void *arg1, void *arg2) {
    if (loader->destroyed) { return; }
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

static void img_loader_free(void *arg, int result) {
    (void) result;
    free(arg);
}