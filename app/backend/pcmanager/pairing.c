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

static int pair_worker(cm_request_t *req);

static void worker_cleanup(cm_request_t *req, int cancelled);

int manual_add_worker(cm_request_t *req);

bool pcmanager_pair(pcmanager_t *manager, SERVER_DATA *server, char *pin, pcmanager_callback_t callback,
                    void *userdata) {
    if (server->paired) return false;
    if (server->currentGame) {
        applog_i("PCManager", "The server %s is in game", server->hostname);
    }
    int pin_num = pin_random(0, 9999);
    SDL_snprintf(pin, 5, "%04d", pin_num);
    cm_request_t *req = cm_request_new(manager, server, callback, userdata);
    req->arg1 = strdup(pin);
    executor_execute(manager->executor, (executor_action_cb) pair_worker,
                     (executor_cleanup_cb) worker_cleanup, req);
    return true;
}

bool pcmanager_manual_add(pcmanager_t *manager, const char *address, pcmanager_callback_t callback, void *userdata) {
    cm_request_t *req = cm_request_new(manager, NULL, callback, userdata);
    req->arg1 = strdup(address);
    executor_execute(manager->executor, (executor_action_cb) manual_add_worker,
                     (executor_cleanup_cb) worker_cleanup, req);
    return true;
}

int pcmanager_upsert_worker(pcmanager_t *manager, const char *address, bool refresh, pcmanager_callback_t callback,
                            void *userdata) {
    char *address_dup = strdup(address);
    pcmanager_list_lock(manager);
    PSERVER_LIST existing = pcmanager_find_by_address(manager, address_dup);
    int ret = 0;
    if (existing) {
        SERVER_STATE_ENUM state = existing->state.code;
        if (state == SERVER_STATE_QUERYING) {
            applog_v("PCManager", "Skip upsert for querying node. address: %s", address_dup);
            pcmanager_list_unlock(manager);
            goto done;
        }
        if (!refresh && state & SERVER_STATE_ONLINE) {
            pcmanager_list_unlock(manager);
            goto done;
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
    ret = gs_init(client, server, address_dup, app_configuration->unsupported);
    address_dup = NULL;
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
        resp->state.code = server->paired ? SERVER_STATE_AVAILABLE : SERVER_STATE_NOT_PAIRED;
        resp->server = server;
        pclist_upsert(manager, resp);
    } else if (existing && ret == GS_IO_ERROR) {
        applog_w("PCManager", "IO error while updating status from %s: %s", server->serverInfo.address, gs_error);
        resp->state.code = SERVER_STATE_OFFLINE;
        resp->server = existing->server;
        pclist_upsert(manager, resp);
    } else {
        resp->state.code = SERVER_STATE_ERROR;
        pcmanager_resp_setgserror(resp, ret, gs_error);
        serverdata_free(server);
    }
    pcmanager_worker_finalize(resp, callback, userdata);
    done:
    if (address_dup) {
        free(address_dup);
    }
    return ret;
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
        resp->state.code = SERVER_STATE_AVAILABLE;
        resp->server = server;
        pclist_upsert(manager, resp);
    } else {
        pcmanager_resp_setgserror(resp, ret, gs_error);
        serverdata_free(server);
    }
    pcmanager_worker_finalize(resp, req->callback, req->userdata);
    return 0;
}

static void worker_cleanup(cm_request_t *req, int cancelled) {
    (void) cancelled;
    if (req->arg1 != NULL) {
        SDL_free(req->arg1);
    }
    SDL_free(req);
}

int manual_add_worker(cm_request_t *req) {
    pcmanager_t *manager = req->manager;
    pcmanager_upsert_worker(manager, req->arg1, true, req->callback, req->userdata);
    return 0;
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "ConstantParameter"
#pragma ide diagnostic ignored "cert-msc50-cpp"

static int pin_random(int min, int max) {
    return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}

#pragma clang diagnostic pop

static void notify_querying(upsert_args_t *args) {
    pcmanager_listeners_notify(args->manager, args->resp, PCMANAGER_NOTIFY_UPDATED);
}