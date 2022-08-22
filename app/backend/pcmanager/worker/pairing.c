#include "worker.h"
#include "errors.h"
#include "app.h"

#include "backend/pcmanager/priv.h"
#include "backend/pcmanager/pclist.h"

#include <errno.h>

int worker_pairing(cm_request_t *context) {
    pcmanager_t *manager = context->manager;
    const pclist_t *node = pcmanager_node(manager, &context->uuid);
    if (node == NULL) {
        return ENOENT;
    }
    GS_CLIENT client = app_gs_client_new();
    PSERVER_DATA server = serverdata_clone(node->server);
    gs_set_timeout(client, 60);
    int ret = gs_pair(client, server, context->arg1);
    gs_destroy(client);
    if (ret != GS_OK) {
        serverdata_free(server);
        return ret;
    }
    SERVER_STATE state = {.code = SERVER_STATE_AVAILABLE};
    pclist_upsert(manager, &context->uuid, &state, server);
    return GS_OK;
}