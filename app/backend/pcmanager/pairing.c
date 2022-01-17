#include "priv.h"
#include "pclist.h"
#include "app.h"
#include "client.h"
#include "errors.h"
#include "util/bus.h"
#include "util/logging.h"
#include "listeners.h"

static int pin_random(int min, int max);

static void notify_querying(upsert_args_t *args);

int pair_worker(cm_request_t *req);

int test_worker(cm_request_t *req);

int manual_add_worker(cm_request_t *req);


bool pcmanager_pair(pcmanager_t *manager, const SERVER_DATA *server, char *pin, pcmanager_callback_t callback,
                    void *userdata) {
    if (server->paired || server->currentGame) return false;
    int pin_num = pin_random(0, 9999);
    SDL_snprintf(pin, 5, "%04d", pin_num);
    cm_request_t *req = cm_request_new(manager, server, callback, userdata);
    req->arg1 = strdup(pin);
    SDL_CreateThread((SDL_ThreadFunction) pair_worker, "pairing", req);
    return true;
}

bool pcmanager_test(pcmanager_t *manager, const SERVER_DATA *server, pcmanager_callback_t callback,
                    void *userdata) {
    cm_request_t *req = cm_request_new(manager, server, callback, userdata);
    SDL_CreateThread((SDL_ThreadFunction) test_worker, "conntest", req);
    return true;
}

bool pcmanager_manual_add(pcmanager_t *manager, const char *address, pcmanager_callback_t callback, void *userdata) {
    cm_request_t *req = cm_request_new(manager, NULL, callback, userdata);
    req->arg1 = strdup(address);
    SDL_CreateThread((SDL_ThreadFunction) manual_add_worker, "add_svr", req);
    return true;
}

int pcmanager_upsert_worker(pcmanager_t *manager, const char *address, bool refresh, pcmanager_callback_t callback,
                            void *userdata) {
    pcmanager_list_lock(manager);
    PSERVER_LIST existing = pcmanager_find_by_address(manager, address);
    if (existing) {
        SERVER_STATE_ENUM state = existing->state.code;
        if (state == SERVER_STATE_QUERYING) {
            applog_v("PCManager", "Skip upsert for querying node. address: %s", address);
            pcmanager_list_unlock(manager);
            return 0;
        }
        if (!refresh && state == SERVER_STATE_ONLINE) {
            pcmanager_list_unlock(manager);
            return 0;
        }
        existing->state.code = SERVER_STATE_QUERYING;
        pcmanager_resp_t resp = {
                .result.code = GS_OK,
                .state = SERVER_STATE_QUERYING,
                .known = existing->known,
                .server = existing->server,
        };
        upsert_args_t args = {.manager = manager, .resp = &resp};
        bus_pushaction_sync((bus_actionfunc) notify_querying, &args);
    }
    pcmanager_list_unlock(manager);
    PSERVER_DATA server = serverdata_new();
    GS_CLIENT client = app_gs_client_new();
    int ret = gs_init(client, server, strdup(address), app_configuration->unsupported);
    gs_destroy(client);
    if (existing) {
        pcmanager_list_lock(manager);
        bool should_remove = ret == GS_OK && SDL_strcasecmp(existing->server->uuid, server->uuid) != 0;
        pcmanager_list_unlock(manager);
        if (should_remove) {
            pclist_remove(manager, existing->server);
            existing = NULL;
        } else {
            pcmanager_list_lock(manager);
            existing->state.code = SERVER_STATE_NONE;
            pcmanager_list_unlock(manager);
        }
    }
    PPCMANAGER_RESP resp = serverinfo_resp_new();
    if (ret == GS_OK) {
        resp->state.code = SERVER_STATE_ONLINE;
        resp->server = server;
        pclist_upsert(manager, resp);
    } else if (existing && ret == GS_IO_ERROR) {
        resp->state.code = SERVER_STATE_OFFLINE;
        resp->server = existing->server;
        pclist_upsert(manager, resp);
    } else {
        resp->state.code = SERVER_STATE_ERROR;
        pcmanager_resp_setgserror(resp, ret, gs_error);
        serverdata_free(server);
    }
    pcmanager_worker_finalize(resp, callback, userdata);
    return ret;
}

static int pin_random(int min, int max) {
    return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}

int pair_worker(cm_request_t *req) {
    pcmanager_t *manager = req->manager;
    GS_CLIENT client = app_gs_client_new();
    PSERVER_DATA server = serverdata_clone(req->server);
    gs_set_timeout(client, 60);
    int ret = gs_pair(client, server, req->arg1);
    gs_destroy(client);

    PPCMANAGER_RESP resp = serverinfo_resp_new();
    if (ret == GS_OK) {
        resp->result.code = GS_OK;
        resp->state.code = SERVER_STATE_ONLINE;
        resp->server = server;
        pclist_upsert(manager, resp);
    } else {
        pcmanager_resp_setgserror(resp, ret, gs_error);
        serverdata_free(server);
    }
    pcmanager_worker_finalize(resp, req->callback, req->userdata);
    SDL_free(req->arg1);
    SDL_free(req);
    return 0;
}

int test_worker(cm_request_t *req) {
    PSERVER_DATA server = serverdata_clone(req->server);
    int ret = 0;

    PPCMANAGER_RESP resp = serverinfo_resp_new();
    if (ret == GS_OK) {
        resp->result.code = GS_OK;
        resp->state.code = SERVER_STATE_ONLINE;
        resp->server = server;
    } else {
        pcmanager_resp_setgserror(resp, ret, gs_error);
    }
    pcmanager_worker_finalize(resp, req->callback, req->userdata);
    serverdata_free(server);
    SDL_free(req);
    return 0;
}

int manual_add_worker(cm_request_t *req) {
    pcmanager_t *manager = req->manager;
    pcmanager_upsert_worker(manager, req->arg1, true, req->callback, req->userdata);
    SDL_free(req->arg1);
    SDL_free(req);
    return 0;
}

static void notify_querying(upsert_args_t *args) {
    pcmanager_listeners_notify(args->manager, args->resp, PCMANAGER_NOTIFY_UPDATED);
}