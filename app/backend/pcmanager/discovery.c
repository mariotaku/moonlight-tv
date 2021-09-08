#include "priv.h"
#include <microdns/microdns.h>
#include "util/logging.h"

typedef struct {
    pcmanager_t *manager;
    SDL_Thread *thread;
    bool stop;
} discovery_task_t;

static int discovery_worker(discovery_task_t *task);

static bool discovery_stop(discovery_task_t *task);

static void discovery_callback(discovery_task_t *task, int status, const struct rr_entry *entries);

static discovery_task_t *discovery_task = NULL;

void pcmanager_auto_discovery_start(pcmanager_t *manager) {
    discovery_task_t *task = SDL_malloc(sizeof(discovery_task_t));
    task->manager = manager;
    task->thread = SDL_CreateThread((SDL_ThreadFunction) discovery_worker, "discovery", task);
    task->stop = false;
    discovery_task = task;
}

void pcmanager_auto_discovery_stop(pcmanager_t *manager) {
    if (!discovery_task) return;
    discovery_task->stop = SDL_TRUE;
}


static int discovery_worker(discovery_task_t *task) {
    int r = 0;
    char err[128];
    struct mdns_ctx *ctx = NULL;
    mdns_init(&ctx, NULL, MDNS_PORT);

    static const char *service_name[] = {"_nvstream._tcp.local"};

    if ((r = mdns_init(&ctx, NULL, MDNS_PORT)) < 0)
        goto err;
    if ((r = mdns_listen(ctx, service_name, 1, RR_PTR, 10, (mdns_stop_func) discovery_stop,
                         (mdns_listen_callback) discovery_callback, task)) < 0)
        goto err;
    err:
    if (r < 0) {
        mdns_strerror(r, err, sizeof(err));
        applog_e("mDNS", "fatal: %s", err);
    }
    mdns_destroy(ctx);
    SDL_free(task);
    discovery_task = NULL;
    return 0;
}

static bool discovery_stop(discovery_task_t *task) {
    return task->stop;
}

static void discovery_callback(discovery_task_t *task, int status, const struct rr_entry *entries) {
    char err[128];

    if (status < 0) {
        mdns_strerror(status, err, sizeof(err));
        applog_e("mDNS", "error: %s", err);
        return;
    }

    for (const struct rr_entry *cur = entries; cur; cur = cur->next) {
        if (cur->type != RR_A) continue;
        pcmanager_upsert_worker(task->manager, cur->data.A.addr_str, NULL, NULL);
    }
}

