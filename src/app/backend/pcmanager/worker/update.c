#include "worker.h"
#include "backend/pcmanager/priv.h"
#include "backend/pcmanager/pclist.h"

#include <assert.h>

#include "errors.h"
#include "util/bus.h"
#include "app.h"
#include "logging.h"
#include "ui/fatal_error.h"

int worker_host_update(worker_context_t *context) {
    const pclist_t *node = pcmanager_node(context->manager, &context->uuid);
    if (node == NULL) {
        return GS_FAILED;
    }
    return pcmanager_update_by_host(context, node->server->serverInfo.address, node->server->extPort, true);
}

int pcmanager_update_by_host(worker_context_t *context, const char *ip, uint16_t port, bool force) {
    assert(context != NULL);
    assert(context->manager != NULL);
    assert(ip != NULL);
    int ret = 0;

    pcmanager_t *manager = context->manager;

    // Fetch server info
    GS_CLIENT client = app_gs_client_new(manager->app);
    SERVER_DATA *server = serverdata_new();
    ret = gs_get_status(client, server, strdup(ip), port, app_configuration->unsupported);
    if (ret == GS_OK) {
        SERVER_STATE state = {.code = server->paired ? SERVER_STATE_AVAILABLE : SERVER_STATE_NOT_PAIRED};
        pclist_upsert(manager, (const uuidstr_t *) server->uuid, &state, server);
    } else {
        const char *error = NULL;
        gs_get_error(&error);
        if (error) {
            context->error = strdup(error);
        }
        serverdata_free(server);
    }
    gs_destroy(client);

    return ret;
}

