#include "priv.h"

#include "util/memlog.h"

PPCMANAGER_RESP serverinfo_resp_new() {
    PPCMANAGER_RESP resp = malloc(sizeof(pcmanager_resp_t));
    SDL_memset(resp, 0, sizeof(pcmanager_resp_t));
    return resp;
}

void pcmanager_worker_finalize(pcmanager_finalizer_args *args) {
    if (args->callback) {
        args->callback(args->resp, args->userdata);
    }
    free(args->resp);
    free(args);
}

pcmanager_finalizer_args *pcmanager_finalize_args(pcmanager_resp_t *resp, pcmanager_callback_t callback,
                                                  void *userdata) {
    pcmanager_finalizer_args *args = malloc(sizeof(pcmanager_finalizer_args));
    args->resp = resp;
    args->callback = callback;
    args->userdata = userdata;
    return args;
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
