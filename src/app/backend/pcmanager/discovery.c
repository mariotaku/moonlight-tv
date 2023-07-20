#include "priv.h"
#include "logging.h"
#include <microdns/microdns.h>
#include "util/bus.h"
#include "backend/pcmanager/worker/worker.h"

struct discovery_task_t {
    pcmanager_t *manager;
    SDL_mutex *lock;
    SDL_Thread *thread;
    bool stop;
};

static int discovery_worker(discovery_task_t *task);

static bool discovery_stop(discovery_task_t *task);

static void discovery_callback(discovery_task_t *task, int status, const struct rr_entry *entries);

static void discovery_finalize(void *arg, int result);

void pcmanager_auto_discovery_start(pcmanager_t *manager) {
    pcmanager_lock(manager);
    if (manager->discovery_task != NULL) {
        pcmanager_unlock(manager);
        return;
    }
    discovery_task_t *task = SDL_calloc(1, sizeof(discovery_task_t));
    commons_log_info("Discovery", "Start task %p", task);
    task->manager = manager;
    task->lock = SDL_CreateMutex();
    task->stop = false;
    task->thread = SDL_CreateThread((SDL_ThreadFunction) discovery_worker, "discovery", task);
    manager->discovery_task = task;
    pcmanager_unlock(manager);
}

void pcmanager_auto_discovery_stop(pcmanager_t *manager) {
    pcmanager_lock(manager);
    discovery_task_t *task = manager->discovery_task;
    if (task == NULL) {
        pcmanager_unlock(manager);
        return;
    }
    manager->discovery_task = NULL;
    executor_submit(manager->executor, executor_noop, discovery_finalize, task);
    pcmanager_unlock(manager);
}

static int discovery_worker(discovery_task_t *task) {
    int r;
    char err[128];
    static const char *const service_name[] = {"_nvstream._tcp.local"};

    struct mdns_ctx *ctx = NULL;
    if ((r = mdns_init(&ctx, NULL, MDNS_PORT)) < 0) {
        goto err;
    }
    commons_log_info("Discovery", "Start mDNS discovery");
    if ((r = mdns_listen(ctx, service_name, 1, RR_PTR, 10, (mdns_stop_func) discovery_stop,
                         (mdns_listen_callback) discovery_callback, task)) < 0) {
        goto err;
    }
    err:
    if (r < 0) {
        mdns_strerror(r, err, sizeof(err));
        commons_log_error("Discovery", "fatal: %s", err);
    }
    if (ctx != NULL) {
        mdns_destroy(ctx);
    }
    commons_log_info("Discovery", "mDNS discovery stopped");
    return r;
}

static bool discovery_stop(discovery_task_t *task) {
    SDL_LockMutex(task->lock);
    bool stop = task->stop;
    SDL_UnlockMutex(task->lock);
    return stop;
}

static void discovery_callback(discovery_task_t *task, int status, const struct rr_entry *entries) {
    char err[128];

    if (status < 0) {
        mdns_strerror(status, err, sizeof(err));
        commons_log_error("Discovery", "error: %s", err);
        return;
    }
    if (task->stop) { return; }
    for (const struct rr_entry *cur = entries; cur != NULL; cur = cur->next) {
        if (cur->type != RR_A) { continue; }
        worker_context_t *ctx = worker_context_new(task->manager, NULL, NULL, NULL);
        ctx->arg1 = strdup(cur->data.A.addr_str);
        pcmanager_worker_queue(task->manager, worker_host_discovered, ctx);
    }
}

static void discovery_finalize(void *arg, int result) {
    (void) result;
    commons_log_info("Discovery", "Finalize task %p", arg);
    discovery_task_t *task = arg;
    SDL_LockMutex(task->lock);
    task->stop = true;
    SDL_UnlockMutex(task->lock);
    SDL_WaitThread(task->thread, NULL);
    SDL_DestroyMutex(task->lock);
    free(task);
}
