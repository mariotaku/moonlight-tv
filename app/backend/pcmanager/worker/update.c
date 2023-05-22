#include "worker.h"
#include "backend/pcmanager/priv.h"
#include "backend/pcmanager/pclist.h"
#include "backend/pcmanager/listeners.h"

#include "errors.h"
#include "util/bus.h"
#include "app.h"
#include "logging.h"

static void notify_querying(pclist_update_context_t *args);

int worker_host_update(worker_context_t *context) {
    const pclist_t *node = pcmanager_node(context->manager, &context->uuid);
    return pcmanager_update_by_ip(context->manager, node->server->serverInfo.address, true);
}

int pcmanager_update_by_ip(pcmanager_t *manager, const char *ip, bool force) {
    SDL_assert(manager != NULL);
    SDL_assert(ip != NULL);
    char *ip_dup = strdup(ip);
    pclist_t *existing = pclist_find_by_ip(manager, ip_dup);
    int ret = 0;
    if (existing) {
        SERVER_STATE_ENUM state = existing->state.code;
        if (state == SERVER_STATE_QUERYING) {
            commons_log_verbose("PCManager", "Skip upsert for querying node. ip: %s", ip_dup);
            goto done;
        }
        if (!force && state & SERVER_STATE_ONLINE) {
            goto done;
        }
        pclist_lock(manager);
        pclist_update_context_t args = {
                .manager = manager,
                .uuid = existing->id,
                .state = {.code = SERVER_STATE_QUERYING},
        };
        existing->state = args.state;
        pclist_unlock(manager);
        bus_pushaction_sync((bus_actionfunc) notify_querying, &args);
    }
    PSERVER_DATA server = serverdata_new();
    GS_CLIENT client = app_gs_client_new();
    ret = gs_init(client, server, ip_dup, app_configuration->unsupported);
    ip_dup = NULL;
    gs_destroy(client);
    if (existing) {
        pclist_lock(manager);
        bool should_remove = ret == GS_OK && !uuidstr_t_equals_s(&existing->id, server->uuid);
        pclist_unlock(manager);
        if (should_remove) {
            pclist_remove(manager, &existing->id);
            existing = NULL;
        } else {
            pclist_lock(manager);
            existing->state.code = SERVER_STATE_NONE;
            pclist_unlock(manager);
        }
    }
    if (ret == GS_OK) {
        uuidstr_t uuid;
        uuidstr_fromstr(&uuid, server->uuid);
        SERVER_STATE state = {.code = server->paired ? SERVER_STATE_AVAILABLE : SERVER_STATE_NOT_PAIRED};
        pclist_upsert(manager, &uuid, &state, server);
    } else {
        if (existing && ret == GS_IO_ERROR) {
            commons_log_warn("PCManager", "IO error while updating status from %s: %s", server->serverInfo.address, gs_error);
            SERVER_STATE state = {.code = SERVER_STATE_OFFLINE};
            pclist_upsert(manager, &existing->id, &state, NULL);
        }
        serverdata_free(server);
    }
    done:
    if (ip_dup) {
        free(ip_dup);
    }
    return ret;
}

static void notify_querying(pclist_update_context_t *args) {
    pcmanager_listeners_notify(args->manager, &args->uuid, PCMANAGER_NOTIFY_UPDATED);
}