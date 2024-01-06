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
    struct sockaddr *addr = context->arg1;
    char ip[64];
    if (sockaddr_get_ip_str(addr, ip, sizeof(ip)) != 0) {
        return GS_FAILED;
    }
    uint16_t port = sockaddr_get_port(addr);
    return pcmanager_update_by_ip(context, ip, port, force);
}
