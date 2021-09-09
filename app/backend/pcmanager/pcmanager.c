#include <errors.h>
#include "backend/pcmanager.h"
#include "priv.h"

#include "util/memlog.h"
#include "pclist.h"
#include "app.h"

cm_request_t *cm_request_new(pcmanager_t *manager, const SERVER_DATA *server, pcmanager_callback_t callback,
                             void *userdata);

static int request_update_worker(cm_request_t *req);

static int quit_app_worker(cm_request_t *req);

pcmanager_t *pcmanager_new() {
    pcmanager_t *manager = SDL_malloc(sizeof(pcmanager_t));
    manager->thread_id = SDL_ThreadID();
    manager->servers_lock = SDL_CreateMutex();
    manager->servers = NULL;
    pcmanager_load_known_hosts(manager);
    return manager;
}

void pcmanager_destroy(pcmanager_t *manager) {
    pcmanager_auto_discovery_stop(manager);
    pcmanager_save_known_hosts(manager);
    pclist_free(manager);
    SDL_DestroyMutex(manager->servers_lock);
    SDL_free(manager);
}

cm_request_t *cm_request_new(pcmanager_t *manager, const SERVER_DATA *server, pcmanager_callback_t callback,
                             void *userdata) {
    cm_request_t *req = malloc(sizeof(cm_request_t));
    req->manager = manager;
    req->server = server;
    req->callback = callback;
    req->userdata = userdata;
    return req;
}

bool pcmanager_quitapp(pcmanager_t *manager, const SERVER_DATA *server, pcmanager_callback_t callback, void *userdata) {
    if (server->currentGame == 0) {
        return false;
    }
    cm_request_t *req = cm_request_new(manager, server, callback, userdata);
    SDL_CreateThread((SDL_ThreadFunction) quit_app_worker, "quitapp", req);
    return true;
}

void pcmanager_request_update(pcmanager_t *manager, const SERVER_DATA *server, pcmanager_callback_t callback,
                              void *userdata) {
    cm_request_t *req = cm_request_new(manager, server, callback, userdata);
    SDL_CreateThread((SDL_ThreadFunction) request_update_worker, "pcupdate", req);
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


PSERVER_LIST pcmanager_servers(pcmanager_t *manager) {
    return manager->servers;
}

static int request_update_worker(cm_request_t *req) {
    pcmanager_upsert_worker(req->manager, req->server->serverInfo.address, true, req->callback, req->userdata);
    free(req);
    return 0;
}

static int quit_app_worker(cm_request_t *req) {
    GS_CLIENT client = app_gs_client_new();
    pcmanager_list_lock(req->manager);
    int ret = gs_quit_app(client, (SERVER_DATA *) req->server);
    pcmanager_list_unlock(req->manager);
    if (ret == GS_OK) {
        pcmanager_resp_t resp = {
                .result.code = ret,
                .state.code = SERVER_STATE_ONLINE,
                .known = true,
                .server = req->server,
        };
        pclist_upsert(req->manager, &resp);
    }
    free(req);
    return 0;
}