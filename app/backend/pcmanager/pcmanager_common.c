#include <util/bus.h>
#include <util/nullable.h>
#include "priv.h"

PPCMANAGER_RESP serverinfo_resp_new() {
    PPCMANAGER_RESP resp = malloc(sizeof(pcmanager_resp_t));
    SDL_memset(resp, 0, sizeof(pcmanager_resp_t));
    return resp;
}

static void finalizer_notify(pcmanager_finalizer_args *args);

void pcmanager_worker_finalize(pcmanager_resp_t *resp, pcmanager_callback_t callback, void *userdata) {
    pcmanager_finalizer_args args = {.resp=resp, .callback=callback, .userdata=userdata};
    bus_pushaction_sync((bus_actionfunc) finalizer_notify, &args);
    free(args.resp);
}


PSERVER_DATA serverdata_new() {
    PSERVER_DATA server = malloc(sizeof(SERVER_DATA));
    SDL_memset(server, 0, sizeof(SERVER_DATA));
    return server;
}

static DISPLAY_MODE *display_mode_clone(const DISPLAY_MODE *mode) {
    if (mode == NULL) return NULL;
    DISPLAY_MODE *result = SDL_malloc(sizeof(DISPLAY_MODE));
    SDL_memcpy(result, mode, sizeof(DISPLAY_MODE));
    result->next = display_mode_clone(mode->next);
    return result;
}

PSERVER_DATA serverdata_clone(const SERVER_DATA *src) {
    PSERVER_DATA server = serverdata_new();
    server->uuid = strdup_nullable(src->uuid);
    server->mac = strdup_nullable(src->mac);
    server->hostname = strdup_nullable(src->hostname);
    server->gpuType = strdup_nullable(src->gpuType);
    server->paired = src->paired;
    server->supports4K = src->supports4K;
    server->supportsHdr = src->supportsHdr;
    server->unsupported = src->unsupported;
    server->currentGame = src->currentGame;
    server->serverMajorVersion = src->serverMajorVersion;
    server->gsVersion = strdup_nullable(src->gsVersion);
    server->modes = display_mode_clone(src->modes);
    server->serverInfo.address = strdup_nullable(src->serverInfo.address);
    server->serverInfo.rtspSessionUrl = strdup_nullable(src->serverInfo.rtspSessionUrl);
    server->serverInfo.serverInfoAppVersion = strdup_nullable(src->serverInfo.serverInfoAppVersion);
    server->serverInfo.serverInfoGfeVersion = strdup_nullable(src->serverInfo.serverInfoGfeVersion);
    return server;
}

void serverdata_free(PSERVER_DATA data) {
    PDISPLAY_MODE mode = data->modes;
    while (mode) {
        PDISPLAY_MODE tmp = mode;
        mode = mode->next;
        SDL_free(tmp);
    }
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

static void finalizer_notify(pcmanager_finalizer_args *args) {
    if (args->callback) {
        args->callback(args->resp, args->userdata);
    }
}