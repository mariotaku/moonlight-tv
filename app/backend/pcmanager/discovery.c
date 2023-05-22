#include "priv.h"
#include "logging.h"
#include <microdns/microdns.h>
#include "util/bus.h"
#include "backend/pcmanager/worker/worker.h"

typedef struct {
    pcmanager_t *manager;
    SDL_Thread *thread;
    bool stop;
} discovery_task_t;

static int discovery_worker(discovery_task_t *task);

static bool discovery_stop(discovery_task_t *task);

static void discovery_callback(discovery_task_t *task, int status, const struct rr_entry *entries);

static void discovery_thread_wait(SDL_Thread *thread);

static discovery_task_t *discovery_task = NULL;

void pcmanager_auto_discovery_start(pcmanager_t *manager) {
    if (discovery_task) return;
    discovery_task_t *task = SDL_malloc(sizeof(discovery_task_t));
    task->manager = manager;
    task->stop = false;
    task->thread = SDL_CreateThread((SDL_ThreadFunction) discovery_worker, "discovery", task);
    discovery_task = task;
}

void pcmanager_auto_discovery_stop(pcmanager_t *manager) {
    if (!discovery_task) return;
    discovery_task->stop = SDL_TRUE;
}

static int discovery_worker(discovery_task_t *task) {
    int r = 0;
    char err[128];
    static const char *const service_name[] = {"_nvstream._tcp.local"};

    struct mdns_ctx *ctx = NULL;
    if ((r = mdns_init(&ctx, NULL, MDNS_PORT)) < 0)
        goto err;
    commons_log_info("Discovery", "Start mDNS discovery");
    if ((r = mdns_listen(ctx, service_name, 1, RR_PTR, 10, (mdns_stop_func) discovery_stop,
                         (mdns_listen_callback) discovery_callback, task)) < 0)
        goto err;
    err:
    if (r < 0) {
        mdns_strerror(r, err, sizeof(err));
        commons_log_error("Discovery", "fatal: %s", err);
    }
    commons_log_info("Discovery", "Stop mDNS discovery");
    if (ctx != NULL) {
        mdns_destroy(ctx);
    }
    SDL_Thread *thread = task->thread;
    SDL_free(task);
    discovery_task = NULL;
    bus_pushaction((bus_actionfunc) discovery_thread_wait, thread);
    return 0;
}

static bool discovery_stop(discovery_task_t *task) {
    return task->stop;
}

static void discovery_callback(discovery_task_t *task, int status, const struct rr_entry *entries) {
    char err[128];

    if (status < 0) {
        mdns_strerror(status, err, sizeof(err));
        commons_log_error("Discovery", "error: %s", err);
        return;
    }
    if (task->stop) return;
    for (const struct rr_entry *cur = entries; cur != NULL; cur = cur->next) {
        if (cur->type != RR_A) continue;
        worker_context_t *ctx = worker_context_new(task->manager, NULL, NULL, NULL);
        ctx->arg1 = strdup(cur->data.A.addr_str);
        pcmanager_worker_queue(task->manager, worker_host_discovered, ctx);
    }
}

static void discovery_thread_wait(SDL_Thread *thread) {
    SDL_WaitThread(thread, NULL);
}