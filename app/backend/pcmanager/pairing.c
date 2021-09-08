#include "priv.h"
#include "pclist.h"
#include "app.h"
#include "client.h"
#include "errors.h"
#include "util/bus.h"

static int pin_random(int min, int max);


static int pin_random(int min, int max) {
    return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}

int pcmanager_upsert_worker(pcmanager_t *manager, const char *address, pcmanager_callback_t callback, void *userdata) {
    pcmanager_list_lock(manager);
    PSERVER_LIST existing = pcmanager_find_by_address(manager, address);
    if (existing) {
        if (existing->state.code == SERVER_STATE_QUERYING) {
            pcmanager_list_unlock(manager);
            return 0;
        }
        existing->state.code = SERVER_STATE_QUERYING;
    }
    pcmanager_list_unlock(manager);
    PSERVER_DATA server = serverdata_new();
    int ret = gs_init(app_gs_client_obtain(), server, strdup(address), app_configuration->unsupported);
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
    } else {
        pcmanager_resp_setgserror(resp, ret, gs_error);
        serverdata_free(server);
    }
    bus_pushaction((bus_actionfunc) pcmanager_worker_finalize, pcmanager_finalize_args(resp, callback, userdata));
    return ret;
}
