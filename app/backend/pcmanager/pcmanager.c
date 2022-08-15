#include <errors.h>
#include "backend/pcmanager.h"
#include "priv.h"

#include "pclist.h"
#include "app.h"

static void request_update_worker(cm_request_t *req);

static void quit_app_worker(cm_request_t *req);

static void pcmanager_free(executor_t *executor);

pcmanager_t *pcmanager_new() {
    pcmanager_t *manager = SDL_calloc(1, sizeof(pcmanager_t));
    manager->executor = executor_create("pcmanager", pcmanager_free);
    manager->thread_id = SDL_ThreadID();
    manager->servers_lock = SDL_CreateMutex();
    executor_set_userdata(manager->executor, manager);
    pcmanager_load_known_hosts(manager);
    return manager;
}

void pcmanager_destroy(pcmanager_t *manager) {
    pcmanager_auto_discovery_stop(manager);
    executor_destroy(manager->executor, 1);
    pcmanager_save_known_hosts(manager);
    pclist_free(manager);
    SDL_DestroyMutex(manager->servers_lock);
    SDL_free(manager);
}

cm_request_t *cm_request_new(pcmanager_t *manager, SERVER_DATA *server, pcmanager_callback_t callback,
                             void *userdata) {
    cm_request_t *req = malloc(sizeof(cm_request_t));
    req->manager = manager;
    req->server = server;
    req->callback = callback;
    req->userdata = userdata;
    return req;
}

bool pcmanager_quitapp(pcmanager_t *manager, SERVER_DATA *server, pcmanager_callback_t callback, void *userdata) {
    if (server->currentGame == 0) {
        return false;
    }
    cm_request_t *req = cm_request_new(manager, server, callback, userdata);
    executor_execute(manager->executor, (executor_action_cb) quit_app_worker, (executor_cleanup_cb) SDL_free, req);
    return true;
}

void pcmanager_request_update(pcmanager_t *manager, SERVER_DATA *server, pcmanager_callback_t callback,
                              void *userdata) {
    cm_request_t *req = cm_request_new(manager, server, callback, userdata);
    executor_execute(manager->executor, (executor_action_cb) request_update_worker, (executor_cleanup_cb) SDL_free,
                     req);
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

static void request_update_worker(cm_request_t *req) {
    pcmanager_upsert_worker(req->manager, req->server->serverInfo.address, true, req->callback, req->userdata);
}

static void quit_app_worker(cm_request_t *req) {
    GS_CLIENT client = app_gs_client_new();
    pcmanager_list_lock(req->manager);
    int ret = gs_quit_app(client, (SERVER_DATA *) req->server);
    pcmanager_list_unlock(req->manager);
    pcmanager_resp_t *resp = serverinfo_resp_new();
    resp->known = true;
    resp->result.code = ret;
    resp->server = req->server;
    if (ret == GS_OK) {
        resp->state.code = SERVER_STATE_AVAILABLE;
        pclist_upsert(req->manager, resp);
    } else {
        resp->result.error.message = gs_error;
    }
    pcmanager_worker_finalize(resp, req->callback, req->userdata);
}

static void pcmanager_free(executor_t *executor) {

}