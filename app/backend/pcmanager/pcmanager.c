#include "backend/pcmanager.h"
#include "priv.h"

#include "util/memlog.h"
#include "pclist.h"


pcmanager_t *computer_manager_new() {
    pcmanager_t *manager = SDL_malloc(sizeof(pcmanager_t));
    manager->thread_id = SDL_ThreadID();
    manager->servers_lock = SDL_CreateMutex();
    manager->servers = NULL;
    pcmanager_load_known_hosts(manager);
    return manager;
}

void computer_manager_destroy(pcmanager_t *manager) {
    pcmanager_auto_discovery_stop(manager);
    pcmanager_save_known_hosts(manager);
    pclist_free(manager);
    SDL_DestroyMutex(manager->servers_lock);
    SDL_free(manager);
}

bool pcmanager_quitapp(const SERVER_DATA *server, pcmanager_callback_t callback, void *userdata) {
    if (server->currentGame == 0) {
        return false;
    }
    cm_request_t *req = malloc(sizeof(cm_request_t));
    req->server = server;
    req->callback = callback;
    req->userdata = userdata;
    // TODO threaded operation
    return true;
}


PPCMANAGER_RESP serverinfo_resp_new() {
    PPCMANAGER_RESP resp = malloc(sizeof(pcmanager_resp_t));
    SDL_memset(resp, 0, sizeof(pcmanager_resp_t));
    return resp;
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
}


PSERVER_LIST pcmanager_servers(pcmanager_t *manager) {
    return manager->servers;
}