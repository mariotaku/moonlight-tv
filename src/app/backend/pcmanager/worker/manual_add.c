#include <assert.h>
#include "worker.h"
#include "backend/pcmanager/priv.h"

static int updated_by_host(worker_context_t *context, bool force);

int worker_add_by_host(worker_context_t *context) {
    assert(context->arg1 != NULL);
    return updated_by_host(context, true);
}

int updated_by_host(worker_context_t *context, bool force) {
    const hostport_t *host = context->arg1;
    return pcmanager_update_by_host(context, hostport_get_hostname(host), hostport_get_port(host), force);
}
