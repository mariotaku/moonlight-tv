#define PCMANAGER_IMPL

#include "backend/pcmanager.h"
#include "priv.h"

#include "app.h"

#include <assert.h>
#include <ctype.h>

#include <SDL.h>

#include <libconfig.h>

#include "libgamestream/errors.h"

#include "util/bus.h"
#include "util/path.h"

#include "stream/session.h"
#include "stream/settings.h"

#include "util/memlog.h"
#include "util/logging.h"
#include "util/libconfig_ext.h"


static pcmanager_listener *callbacks_list;

static int _pcmanager_quitapp_action(void *data);

static int _computer_manager_server_update_action(PSERVER_DATA data);

pcmanager_t *computer_manager_new() {
    pcmanager_t *manager = SDL_malloc(sizeof(pcmanager_t));
    manager->servers_lock = SDL_CreateMutex();
    manager->servers = NULL;
    pcmanager_load_known_hosts(manager);
    return manager;
}

void computer_manager_destroy(pcmanager_t *manager) {
    pcmanager_auto_discovery_stop(manager);
    pcmanager_save_known_hosts(manager);
    serverlist_free(manager->servers, serverlist_nodefree);
    SDL_DestroyMutex(manager->servers_lock);
    SDL_free(manager);
}

void handle_server_updated(PPCMANAGER_RESP update) {
    assert(update);
    if (update->result.code != GS_OK || !update->server)
        return;
    PSERVER_LIST node = serverlist_find_by(pcmanager->servers, update->server->uuid, serverlist_compare_uuid);
    if (!node)
        return;
    if (update->server_shallow)
        free((PSERVER_DATA) node->server);
    else
        serverdata_free((PSERVER_DATA) node->server);

    serverlist_set_from_resp(node, update);
}

void handle_server_queried(PPCMANAGER_RESP resp) {
    if (resp->state.code != SERVER_STATE_ONLINE)
        return;
    PSERVER_LIST node = serverlist_find_by(pcmanager->servers, resp->server->uuid, serverlist_compare_uuid);
    if (node) {
        if (node->server) {
            serverdata_free((PSERVER_DATA) node->server);
        }
        serverlist_set_from_resp(node, resp);
        for (pcmanager_listener *cur = callbacks_list; cur != NULL; cur = cur->next) {
            if (!cur->updated) continue;
            cur->updated(cur->userdata, resp);
        }
    } else {
        node = serverlist_new();
        serverlist_set_from_resp(node, resp);

        pcmanager->servers = serverlist_append(pcmanager->servers, node);
        for (pcmanager_listener *cur = callbacks_list; cur != NULL; cur = cur->next) {
            if (!cur->added) continue;
            cur->added(cur->userdata, resp);
        }
    }
}

bool pcmanager_quitapp(const SERVER_DATA *server, pcmanager_callback_t callback, void *userdata) {
    if (server->currentGame == 0) {
        return false;
    }
    cm_request_t *req = malloc(sizeof(cm_request_t));
    req->server = server;
    req->callback = callback;
    req->userdata = userdata;
    SDL_CreateThread(_pcmanager_quitapp_action, "quitapp", req);
    return true;
}

int _pcmanager_quitapp_action(void *data) {
    cm_request_t *req = (cm_request_t *) data;
    PPCMANAGER_RESP resp = serverinfo_resp_new();
    PSERVER_DATA server = serverdata_new();
    SDL_memcpy(server, req->server, sizeof(SERVER_DATA));
    int ret = gs_quit_app(app_gs_client_obtain(), server);
    if (ret != GS_OK)
        pcmanager_resp_setgserror(resp, ret, gs_error);
    resp->server = server;
    resp->server_shallow = true;
    bus_pushaction((bus_actionfunc) handle_server_updated, resp);
    bus_pushaction((bus_actionfunc) invoke_callback, invoke_callback_args(resp, req->callback, req->userdata));
    bus_pushaction((bus_actionfunc) serverinfo_resp_free, resp);
    free(req);
    return 0;
}

int _computer_manager_server_update_action(PSERVER_DATA data) {
    PSERVER_DATA server = serverdata_new();
    PPCMANAGER_RESP update = serverinfo_resp_new();
    int ret = gs_init(app_gs_client_obtain(), server, SDL_strdup(data->serverInfo.address),
                      app_configuration->unsupported);
    if (ret == GS_OK) {
        update->state.code = SERVER_STATE_ONLINE;
        update->server = server;
        update->server_shallow = false;
    } else {
        serverstate_setgserror(&update->state, ret, gs_error);
    }
    bus_pushaction((bus_actionfunc) handle_server_updated, update);
    bus_pushaction((bus_actionfunc) serverinfo_resp_free, update);
    free(data);
    return 0;
}


PPCMANAGER_RESP serverinfo_resp_new() {
    PPCMANAGER_RESP resp = malloc(sizeof(PCMANAGER_RESP));
    SDL_memset(resp, 0, sizeof(PCMANAGER_RESP));
    return resp;
}

void serverinfo_resp_free(PPCMANAGER_RESP resp) {
    if (!resp->server_referenced && resp->server) {
        free((void *) resp->server);
    }
    free(resp);
}

void serverstate_setgserror(SERVER_STATE *state, int code, const char *msg) {
    state->code = SERVER_STATE_ERROR;
    state->error.errcode = code;
    state->error.errmsg = msg;
}

void pcmanager_resp_setgserror(PPCMANAGER_RESP resp, int code, const char *msg) {
    resp->result.code = code;
    resp->result.error.message = msg;
}

void pcmanager_request_update(const SERVER_DATA *server) {
    PSERVER_DATA arg = serverdata_new();
    SDL_memcpy(arg, server, sizeof(SERVER_DATA));
    SDL_CreateThread((SDL_ThreadFunction) _computer_manager_server_update_action, "srvupd", arg);
}

void pcmanager_register_listener(pcmanager_listener *listener) {
    listener->next = NULL;
    listener->prev = NULL;
    callbacks_list = pcmanager_callbacks_append(callbacks_list, listener);
}

static int pcmanager_callbacks_comparator(pcmanager_listener *p1, const void *p2) {
    return p1 != p2;
}

void pcmanager_unregister_listener(pcmanager_listener *listener) {
    assert(listener);
    pcmanager_listener *find = pcmanager_callbacks_find_by(callbacks_list, listener, pcmanager_callbacks_comparator);
    if (!find)
        return;
    callbacks_list = pcmanager_callbacks_remove(callbacks_list, find);
}


PSERVER_LIST pcmanager_servers(pcmanager_t *self) {
    return self->servers;
}