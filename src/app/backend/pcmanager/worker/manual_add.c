#include "worker.h"
#include "backend/pcmanager/priv.h"
#include "errors.h"
#include "sockaddr.h"

static int updated_by_addr(worker_context_t *context, bool force);

int worker_add_by_ip(worker_context_t *context) {
    return updated_by_addr(context, true);
}

int worker_host_discovered(worker_context_t *context) {
    return updated_by_addr(context, false);
}

int updated_by_addr(worker_context_t *context, bool force) {
    return pcmanager_update_by_addr(context, context->arg1, force);
}
