#include "worker.h"
#include "backend/pcmanager/priv.h"

int worker_add_by_ip(worker_context_t *context) {
    return pcmanager_update_by_ip(context, context->arg1, true);
}

int worker_host_discovered(worker_context_t *context) {
    return pcmanager_update_by_ip(context, context->arg1, false);
}