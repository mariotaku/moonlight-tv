#pragma once

#include "backend/pcmanager.h"

typedef struct app_t app_t;
typedef struct worker_context_t {
    app_t *app;
    pcmanager_t *manager;
    uuidstr_t uuid;
    void *arg1;

    int result;
    char *error;

    pcmanager_callback_t callback;
    void *userdata;
} worker_context_t;

typedef int (*worker_action)(worker_context_t *context);

int worker_pairing(worker_context_t *context);

int worker_quit_app(worker_context_t *context);

int worker_wol(worker_context_t *context);

int worker_add_by_ip(worker_context_t *context);

int worker_host_discovered(worker_context_t *context);

int worker_host_update(worker_context_t *context);

worker_context_t *worker_context_new(pcmanager_t *manager, const uuidstr_t *uuid, pcmanager_callback_t callback,
                                     void *userdata);

void worker_context_finalize(worker_context_t *context, int result);

void pcmanager_worker_queue(pcmanager_t *manager, worker_action action, worker_context_t *context);