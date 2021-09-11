#include "priv.h"
#include "pclist.h"
#include "app.h"
#include "client.h"
#include "errors.h"
#include "util/bus.h"
#include "util/logging.h"

static int pin_random(int min, int max);


int pair_worker(cm_request_t *req);


bool pcmanager_pair(pcmanager_t *manager, const SERVER_DATA *server, char *pin, pcmanager_callback_t callback,
                    void *userdata) {
    if (server->paired) return false;
    int pin_num = pin_random(0, 9999);
    SDL_snprintf(pin, 5, "%04d", pin_num);
    cm_request_t *req = cm_request_new(manager, server, callback, userdata);
    req->arg1 = strdup(pin);
    SDL_CreateThread((SDL_ThreadFunction) pair_worker, "pairing", req);
    return true;
}

int pcmanager_upsert_worker(pcmanager_t *manager, const char *address, bool refresh, pcmanager_callback_t callback,
                            void *userdata) {
    pcmanager_list_lock(manager);
    PSERVER_LIST existing = pcmanager_find_by_address(manager, address);
    if (existing) {
        if (existing->state.code == SERVER_STATE_QUERYING) {
            applog_v("PCManager", "Skip upsert for querying node. address: %s", address);
            pcmanager_list_unlock(manager);
            return 0;
        }
        if (!refresh && existing->state.code == SERVER_STATE_ONLINE) {
            pcmanager_list_unlock(manager);
            return 0;
        }
        existing->state.code = SERVER_STATE_QUERYING;
    }
    pcmanager_list_unlock(manager);
    PSERVER_DATA server = serverdata_new();
    GS_CLIENT client = app_gs_client_new();
    int ret = gs_init(client, server, strdup(address), app_configuration->unsupported);
    gs_destroy(client);
    if (existing) {
        pcmanager_list_lock(manager);
        existing->state.code = SERVER_STATE_NONE;
        pcmanager_list_unlock(manager);
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
    gs_set_timeout(client, 60);
    PSERVER_DATA server = serverdata_clone(req->server);
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