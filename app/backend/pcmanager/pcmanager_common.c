#include "priv.h"
#include "app.h"
#include "libgamestream/client.h"
#include "libgamestream/errors.h"
#include "util/bus.h"
#include <SDL.h>

int pcmanager_insert_by_address(const char *srvaddr, bool pair, pcmanager_callback_t callback, void *userdata) {
    PSERVER_DATA server = serverdata_new();
    int ret = gs_init(app_gs_client_obtain(), server, srvaddr, app_configuration->unsupported);

    PPCMANAGER_RESP resp = serverinfo_resp_new();
    if (ret == GS_OK) {
        resp->state.code = SERVER_STATE_ONLINE;
        resp->server = server;
        if (server->paired) {
            resp->known = true;
        }
        resp->server = server;
        bus_pushaction((bus_actionfunc) handle_server_discovered, resp);
    } else {
        pcmanager_resp_setgserror(resp, ret, gs_error);
        free(server);
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

bool pcmanager_is_known_host(const char *srvaddr) {
    assert(srvaddr);
    PSERVER_LIST find = serverlist_find_by(computer_list, srvaddr, serverlist_find_address);
    return find != NULL && find->known;
}