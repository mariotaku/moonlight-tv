#include "worker.h"
#include "backend/pcmanager/priv.h"
#include "errors.h"
#include "sockaddr.h"

int worker_add_by_ip(worker_context_t *context) {
    return pcmanager_update_by_ip(context, context->arg1, 0, true);
}

int worker_host_discovered(worker_context_t *context) {
    struct sockaddr *addr = context->arg1;
    char ip[64];
    if (sockaddr_address_to_string(addr, ip, sizeof(ip)) != 0) {
        return GS_FAILED;
    }
    uint16_t port = sockaddr_get_port(addr);
    return pcmanager_update_by_ip(context, ip, port, false);
}