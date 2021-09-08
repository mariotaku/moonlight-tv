#include "priv.h"
#include "app.h"
#include "libgamestream/client.h"
#include "libgamestream/errors.h"
#include "util/bus.h"
#include <SDL.h>


int query_server(const char *address, pcmanager_callback_t callback, void *userdata);

int pcmanager_insert_sync(const char *address, pcmanager_callback_t callback, void *userdata) {
    pcmanager_list_lock(pcmanager);
    PSERVER_LIST existing = pcmanager_find_by_address(address);
    if (existing && existing->state.code == SERVER_STATE_QUERYING) {
        pcmanager_list_unlock(pcmanager);
        return 0;
    }
    existing->state.code = SERVER_STATE_QUERYING;
    pcmanager_list_unlock(pcmanager);
    return query_server(address, callback, userdata);
}

int pcmanager_update_sync(PSERVER_LIST existing, pcmanager_callback_t callback, void *userdata) {
    pcmanager_list_lock(pcmanager);
    if (existing && existing->state.code == SERVER_STATE_QUERYING) {
        pcmanager_list_unlock(pcmanager);
        return 0;
    }
    existing->state.code = SERVER_STATE_QUERYING;
    pcmanager_list_unlock(pcmanager);
    return query_server(existing->server->serverInfo.address, callback, userdata);
}

int query_server(const char *address, pcmanager_callback_t callback, void *userdata) {
    PSERVER_DATA server = serverdata_new();
    int ret = gs_init(app_gs_client_obtain(), server, strdup(address), app_configuration->unsupported);

    PPCMANAGER_RESP resp = serverinfo_resp_new();
    if (ret == GS_OK) {
        resp->state.code = SERVER_STATE_ONLINE;
        resp->server = server;
        if (server->paired) {
            resp->known = true;
        }
        resp->server = server;
        bus_pushaction((bus_actionfunc) handle_server_queried, resp);
    } else {
        pcmanager_resp_setgserror(resp, ret, gs_error);
        serverdata_free(server);
    }
    if (callback) {
        bus_pushaction((bus_actionfunc) invoke_callback, invoke_callback_args(resp, callback, userdata));
    }
    bus_pushaction((bus_actionfunc) serverinfo_resp_free, resp);
    return ret;
}

void invoke_callback(invoke_callback_t *args) {
    args->callback(args->resp, args->userdata);
    SDL_free(args);
}

invoke_callback_t *invoke_callback_args(PPCMANAGER_RESP resp, pcmanager_callback_t callback, void *userdata) {
    invoke_callback_t *args = SDL_malloc(sizeof(invoke_callback_t));
    args->resp = resp;
    args->callback = callback;
    args->userdata = userdata;
    return args;
}


static int serverlist_find_address(PSERVER_LIST other, const void *v) {
    return SDL_strcmp(other->server->serverInfo.address, (char *) v);
}

int serverlist_compare_uuid(PSERVER_LIST other, const void *v) {
    return SDL_strcasecmp(v, other->server->uuid);
}


PSERVER_LIST pcmanager_find_by_address(const char *srvaddr) {
    SDL_assert(srvaddr);
    return serverlist_find_by(pcmanager->servers, srvaddr, serverlist_find_address);
}

PSERVER_DATA serverdata_new() {
    PSERVER_DATA server = malloc(sizeof(SERVER_DATA));
    SDL_memset(server, 0, sizeof(SERVER_DATA));
    return server;
}

static inline void free_nullable(void *p) {
    if (!p) return;
    free(p);
}

void serverdata_free(PSERVER_DATA data) {
    free_nullable(data->modes);
    free_nullable((void *) data->uuid);
    free_nullable((void *) data->mac);
    free_nullable((void *) data->hostname);
    free_nullable((void *) data->gpuType);
    free_nullable((void *) data->gsVersion);
    free_nullable((void *) data->serverInfo.serverInfoAppVersion);
    free_nullable((void *) data->serverInfo.serverInfoGfeVersion);
    free_nullable((void *) data->serverInfo.address);
    free(data);
}
