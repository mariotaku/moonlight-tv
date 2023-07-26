#include "worker.h"
#include "backend/pcmanager/priv.h"
#include "backend/pcmanager/pclist.h"
#include "backend/pcmanager/listeners.h"

#include "errors.h"
#include "util/bus.h"
#include "app.h"
#include "logging.h"
#include "ui/fatal_error.h"

static void notify_querying(pclist_update_context_t *args);

int worker_host_update(worker_context_t *context) {
    const pclist_t *node = pcmanager_node(context->manager, &context->uuid);
    return pcmanager_update_by_ip(context, node->server->serverInfo.address, true);
}

int pcmanager_update_by_ip(worker_context_t *context, const char *ip, bool force) {
    SDL_assert_release(context != NULL);
    SDL_assert_release(context->manager != NULL);
    SDL_assert_release(ip != NULL);
    pcmanager_t *manager = context->manager;
    char *ip_dup = strdup(ip);
    pclist_t *existing = pclist_find_by_ip(manager, ip_dup);
    if (existing) {
        SERVER_STATE_ENUM state = existing->state.code;
        if (state == SERVER_STATE_QUERYING) {
            commons_log_verbose("PCManager", "Skip upsert for querying node. ip: %s", ip_dup);
            goto done;
        }
        if (!force && state & SERVER_STATE_ONLINE) {
            goto done;
        }
        pcmanager_lock(manager);
        pclist_update_context_t args = {
                .manager = manager,
                .uuid = existing->id,
                .state = {.code = SERVER_STATE_QUERYING},
        };
        existing->state = args.state;
        pcmanager_unlock(manager);
        app_bus_post_sync(context->app, (bus_actionfunc) notify_querying, &args);
    }
    GS_CLIENT client = app_gs_client_new(context->app);
    PSERVER_DATA server = serverdata_new();
    int ret = gs_get_status(client, server, ip_dup, app_configuration->unsupported);
    ip_dup = NULL;
    gs_destroy(client);
    if (existing) {
        pcmanager_lock(manager);
        bool should_remove = ret == GS_OK && !uuidstr_t_equals_s(&existing->id, server->uuid);
        pcmanager_unlock(manager);
        if (should_remove) {
            pclist_remove(manager, &existing->id);
            existing = NULL;
        } else {
            pcmanager_lock(manager);
            existing->state.code = SERVER_STATE_NONE;
            pcmanager_unlock(manager);
        }
    }
    if (ret == GS_OK) {
        commons_log_info("PCManager", "Finished updating status from %s", server->serverInfo.address);
        uuidstr_t uuid;
        uuidstr_fromstr(&uuid, server->uuid);
        SERVER_STATE state = {.code = server->paired ? SERVER_STATE_AVAILABLE : SERVER_STATE_NOT_PAIRED};
        pclist_upsert(manager, &uuid, &state, server);
    } else {
        const char *gs_error = NULL;
        ret = gs_get_error(&gs_error);
        commons_log_warn("PCManager", "Error %d while updating status from %s: %s", ret, server->serverInfo.address,
                         gs_error);
        context->error = gs_error != NULL ? strdup(gs_error) : NULL;
        if (existing && ret == GS_IO_ERROR) {
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