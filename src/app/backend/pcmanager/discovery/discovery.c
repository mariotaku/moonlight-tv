#include "discovery.h"
#include "throttle.h"

#include "impl/impl.h"

#include "util/bus.h"
#include "logging.h"

static int discovery_worker_wrapper(void *arg);

void discovery_init(discovery_t *discovery, discovery_callback callback, void *user_data) {
    discovery->lock = SDL_CreateMutex();
    discovery->task = NULL;
    discovery_throttle_init(&discovery->throttle, callback, user_data);
}

void discovery_deinit(discovery_t *discovery) {
    discovery_throttle_deinit(&discovery->throttle);
    discovery_stop(discovery);
    SDL_DestroyMutex(discovery->lock);
}

void discovery_start(discovery_t *discovery) {
    SDL_LockMutex(discovery->lock);
    if (discovery->task != NULL) {
        SDL_UnlockMutex(discovery->lock);
        return;
    }
    discovery_task_t *task = SDL_calloc(1, sizeof(discovery_task_t));
    commons_log_info("Discovery", "Start task %p", task);
    task->discovery = discovery;
    task->lock = SDL_CreateMutex();
    task->stop = false;
    SDL_Thread *thread = SDL_CreateThread(discovery_worker_wrapper, "discovery", task);
    SDL_DetachThread(thread);
    discovery->task = task;
    SDL_UnlockMutex(discovery->lock);
}

void discovery_stop(discovery_t *discovery) {
    SDL_LockMutex(discovery->lock);
    discovery_task_t *task = discovery->task;
    if (task == NULL) {
        SDL_UnlockMutex(discovery->lock);
        return;
    }
    discovery->task = NULL;
    discovery_worker_stop(task);
    SDL_UnlockMutex(discovery->lock);
}

void discovery_discovered(struct discovery_t *discovery, const sockaddr_t *addr) {
    discovery_throttle_on_discovered(&discovery->throttle, addr, 10000);
}

int discovery_worker_wrapper(void *arg) {
    discovery_task_t *task = (discovery_task_t *) arg;
    int result = discovery_worker(task);
    SDL_DestroyMutex(task->lock);
    free(arg);
    return result;
}