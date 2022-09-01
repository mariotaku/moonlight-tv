#include "worker.h"
#include "backend/pcmanager/priv.h"
#include "util/bus.h"

#include <errno.h>
#include <stdlib.h>

static void worker_callback(worker_context_t *args);

worker_context_t *worker_context_new(pcmanager_t *manager, const uuidstr_t *uuid, pcmanager_callback_t callback,
                                     void *userdata) {
    worker_context_t *context = calloc(1, sizeof(worker_context_t));
    context->manager = manager;
    if (uuid != NULL) {
        context->uuid = *uuid;
    }
    context->callback = callback;
    context->userdata = userdata;
    return context;
}

void worker_context_finalize(worker_context_t *context, int result) {
    context->result = result;
    if (result != ECANCELED) {
        bus_pushaction_sync((bus_actionfunc) worker_callback, context);
    }
    if (context->arg1 != NULL) {
        free(context->arg1);
    }
    free(context);
}

void pcmanager_worker_queue(pcmanager_t *manager, worker_action action, worker_context_t *context) {
    executor_execute(manager->executor, (executor_action_cb) action,
                     (executor_cleanup_cb) worker_context_finalize, context);
}

static void worker_callback(worker_context_t *args) {
    if (args->callback) {
        args->callback(args->result, args->error, &args->uuid, args->userdata);
    }
}