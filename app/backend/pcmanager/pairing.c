#include "backend/pcmanager.h"
#include "priv.h"
#include "app.h"

#include <stdlib.h>
#include <string.h>

#include <SDL.h>
#include <pthread.h>
#include <signal.h>

#include "libgamestream/errors.h"

#include "util/bus.h"
#include "util/user_event.h"

#include "util/memlog.h"

static void *_pcmanager_pairing_action(cm_request_t *req);

static void *_pcmanager_unpairing_action(cm_request_t *req);

static void *_manual_adding_action(cm_request_t *req);

static int pin_random(int min, int max);

bool pcmanager_pair(const SERVER_DATA *server, char *pin, pcmanager_callback_t callback, void *userdata) {
    int pin_int = pin_random(0, 9999);
    cm_request_t *req = malloc(sizeof(cm_request_t));
    SDL_snprintf(pin, 5, "%04u", pin_int);
    req->arg1 = SDL_strdup(pin);
    req->server = server;
    req->callback = callback;
    req->userdata = userdata;
    SDL_CreateThread((SDL_ThreadFunction) _pcmanager_pairing_action, "pairing", req);
    return true;
}

bool pcmanager_unpair(const SERVER_DATA *server, pcmanager_callback_t callback, void *userdata) {
    cm_request_t *req = malloc(sizeof(cm_request_t));
    req->server = server;
    req->callback = callback;
    req->userdata = userdata;
    SDL_CreateThread((SDL_ThreadFunction) _pcmanager_unpairing_action, "unpairing", req);
    return true;
}

bool pcmanager_manual_add(const char *address, pcmanager_callback_t callback, void *userdata) {
    cm_request_t *req = malloc(sizeof(cm_request_t));
    req->arg1 = address;
    req->callback = callback;
    req->userdata = userdata;
    SDL_CreateThread((SDL_ThreadFunction) _manual_adding_action, "adding", req);
    return true;
}

void *_pcmanager_pairing_action(cm_request_t *req) {
    PPCMANAGER_RESP resp = serverinfo_resp_new();
    PSERVER_DATA server = serverdata_new();
    SDL_memcpy(server, req->server, sizeof(SERVER_DATA));
    int ret = gs_pair(app_gs_client_obtain(), server, (char *) req->arg1);
    if (ret == GS_OK) {
        resp->known = true;
    } else {
        pcmanager_resp_setgserror(resp, ret, gs_error);
    }
    resp->server = server;
    resp->server_shallow = true;
    bus_pushaction((bus_actionfunc) handle_server_updated, resp);
    bus_pushaction((bus_actionfunc) invoke_callback, invoke_callback_args(resp, req->callback, req->userdata));
    bus_pushaction((bus_actionfunc) serverinfo_resp_free, resp);
    free(req);
    return NULL;
}

void *_pcmanager_unpairing_action(cm_request_t *req) {
    PPCMANAGER_RESP resp = serverinfo_resp_new();
    PSERVER_DATA server = serverdata_new();
    SDL_memcpy(server, req->server, sizeof(SERVER_DATA));
    int ret = gs_unpair(app_gs_client_obtain(), server);
    if (ret == GS_OK)
        resp->known = false;
    else
        pcmanager_resp_setgserror(resp, ret, gs_error);
    resp->server = server;
    resp->server_shallow = true;
    bus_pushaction((bus_actionfunc) handle_server_updated, resp);
    bus_pushaction((bus_actionfunc) invoke_callback, invoke_callback_args(resp, req->callback, req->userdata));
    bus_pushaction((bus_actionfunc) serverinfo_resp_free, resp);
    free(req);
    return NULL;
}

void *_manual_adding_action(cm_request_t *req) {
    pcmanager_insert_by_address((const char *) req->arg1, true, req->callback, req->userdata);
    free(req);
    return NULL;
}

static int pin_random(int min, int max) {
    return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}
