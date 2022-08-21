#include "worker.h"
#include "backend/pcmanager/priv.h"

int worker_add_by_ip(cm_request_t *context) {
    pcmanager_t *manager = context->manager;
    pcmanager_update_by_ip(manager, context->arg1, true);
    return 0;
}

int worker_host_discovered(worker_context_t *context) {
    pcmanager_t *manager = context->manager;
    pcmanager_update_by_ip(manager, context->arg1, false);
    return 0;
}