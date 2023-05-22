#include "executor.h"

#include "logging.h"

#include <stdlib.h>
#include <errno.h>
#include <SDL2/SDL.h>

typedef struct executor_task_t {
    executor_action_cb action;
    executor_cleanup_cb finalize;
    void *arg;
    SDL_bool cancelled;
    struct executor_task_t *next;
} executor_task_t;

struct executor_t {
    SDL_Thread *thread;
    SDL_mutex *lock;
    SDL_cond *cond;
    executor_task_t *queue;
    executor_finalize_cb finalize;
    executor_task_t *active;
    SDL_bool destroyed;
    SDL_bool wait;

    void *userdata;
};

#define LINKEDLIST_IMPL
#define LINKEDLIST_TYPE executor_task_t
#define LINKEDLIST_PREFIX tasks
#define LINKEDLIST_DOUBLE 0

#include "util/linked_list.h"
#include "bus.h"

#undef LINKEDLIST_TYPE
#undef LINKEDLIST_PREFIX
#undef LINKEDLIST_DOUBLE

static int thread_worker(void *context);

static executor_task_t *tasks_poll(executor_t *executor);

static int task_identical(executor_task_t *p, const void *fv);

static void task_destroy(executor_task_t *task);

static void thread_wait(SDL_Thread *thread);

static void thread_wait_async(SDL_Thread *thread);

executor_t *executor_create(const char *name, executor_finalize_cb finalize_fn) {
    executor_t *executor = calloc(1, sizeof(executor_t));
    executor->destroyed = SDL_FALSE;
    executor->lock = SDL_CreateMutex();
    executor->cond = SDL_CreateCond();
    executor->finalize = finalize_fn;
    executor->thread = SDL_CreateThread(thread_worker, name, executor);
    return executor;
}

void executor_destroy(executor_t *executor, int wait) {
    SDL_assert(!executor->destroyed);
    SDL_LockMutex(executor->lock);
    executor->wait = wait;
    executor->destroyed = SDL_TRUE;
    SDL_CondSignal(executor->cond);
    SDL_UnlockMutex(executor->lock);
    if (wait) {
        thread_wait(executor->thread);
        if (executor->finalize != NULL) {
            executor->finalize(executor);
        }
        free(executor);
    }
}

const executor_task_t *executor_execute(executor_t *executor, executor_action_cb action, executor_cleanup_cb finalize,
                                        void *arg) {
    SDL_LockMutex(executor->lock);
    executor_task_t *task = calloc(1, sizeof(executor_task_t));
    task->action = action;
    task->finalize = finalize;
    task->arg = arg;
    executor->queue = tasks_append(executor->queue, task);
    SDL_CondSignal(executor->cond);
    SDL_UnlockMutex(executor->lock);
    return task;
}

void executor_cancel(executor_t *executor, const executor_task_t *task) {
    SDL_LockMutex(executor->lock);
    if (task != NULL) {
        if (task == executor->active) {
            executor->active->cancelled = SDL_TRUE;
        } else {
            executor_task_t *queue_task = tasks_find_by(executor->queue, task, task_identical);
            if (queue_task) {
                queue_task->cancelled = SDL_TRUE;
            }
        }
    } else {
        if (executor->active != NULL) {
            executor->active->cancelled = SDL_TRUE;
        }
        for (executor_task_t *cur = executor->queue; cur; cur = cur->next) {
            cur->cancelled = SDL_TRUE;
        }
    }
    SDL_UnlockMutex(executor->lock);
}

void *executor_get_userdata(executor_t *executor) {
    return executor->userdata;
}

void executor_set_userdata(executor_t *executor, void *userdata) {
    executor->userdata = userdata;
}

int executor_is_cancelled(const executor_t *executor, const executor_task_t *task) {
    SDL_bool cancelled = SDL_FALSE;
    SDL_LockMutex(executor->lock);
    executor_task_t *queue_task = tasks_find_by(executor->queue, task, task_identical);
    if (queue_task) {
        cancelled = queue_task->cancelled;
    }
    SDL_UnlockMutex(executor->lock);
    return cancelled;
}

int executor_is_active(const executor_t *executor) {
    return executor->active != NULL;
}

int executor_is_destroyed(const executor_t *executor) {
    return executor->destroyed;
}

static executor_task_t *tasks_poll(executor_t *executor) {
    SDL_LockMutex(executor->lock);
    while (executor->queue == NULL) {
        if (executor->destroyed) {
            return NULL;
        }
        SDL_CondWait(executor->cond, executor->lock);
    }
    executor_task_t *task = executor->queue;
    SDL_assert(task);
    executor->queue = task->next;
    SDL_UnlockMutex(executor->lock);
    return task;
}

static int thread_worker(void *context) {
    executor_t *executor = context;
    for (executor_task_t *task = NULL; (task = tasks_poll(executor)) != NULL;) {
        executor->active = task;
        int result = task->action(task->arg);
        if (task->finalize) {
            task->finalize(task->arg, executor->destroyed || task->cancelled ? ECANCELED : result);
        }
        free(task);
        executor->active = NULL;
    }

    SDL_LockMutex(executor->lock);
    tasks_free(executor->queue, task_destroy);
    executor->queue = NULL;
    SDL_UnlockMutex(executor->lock);

    SDL_DestroyCond(executor->cond);
    SDL_DestroyMutex(executor->lock);
    executor->cond = NULL;
    executor->lock = NULL;
    if (!executor->wait) {
        thread_wait_async(executor->thread);
        if (executor->finalize != NULL) {
            executor->finalize(executor);
        }
        free(executor);
    }
    return 0;
}

static int task_identical(executor_task_t *p, const void *fv) {
    return (void *) p == fv;
}

static void task_destroy(executor_task_t *task) {
    if (task->finalize) {
        task->finalize(task->arg, ECANCELED);
    }
    free(task);
}

static void thread_wait(SDL_Thread *thread) {
    const char *name = SDL_GetThreadName(thread);
    commons_log_debug("Executor", "Freeing thread %s", name ? name : "unnamed");
    SDL_WaitThread(thread, NULL);
}

static void thread_wait_async(SDL_Thread *thread) {
    bus_pushaction((bus_actionfunc) thread_wait, thread);
}