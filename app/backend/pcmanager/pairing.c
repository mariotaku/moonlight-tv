#include "backend/computer_manager.h"
#include "priv.h"

#include <stdlib.h>
#include <string.h>

#include <pthread.h>
#include <signal.h>

#include "libgamestream/errors.h"

#include "util/bus.h"
#include "util/user_event.h"

#include "util/memlog.h"

static void *_pcmanager_pairing_action(cm_pin_request *req);
static void *_pcmanager_unpairing_action(cm_pin_request *req);
static void *_manual_adding_action(cm_pin_request *req);
static int pin_random(int min, int max);

bool pcmanager_pair(const SERVER_DATA *server, char *pin, void (*callback)(PPCMANAGER_RESP))
{
    int pin_int = pin_random(0, 9999);
    cm_pin_request *req = malloc(sizeof(cm_pin_request));
    snprintf(pin, 5, "%04u", pin_int);
    req->arg1 = strdup(pin);
    req->server = server;
    req->callback = callback;
    pthread_t pair_thread;
    pthread_create(&pair_thread, NULL, (void *(*)(void *))_pcmanager_pairing_action, req);
    return true;
}

bool pcmanager_unpair(const SERVER_DATA *server, void (*callback)(PPCMANAGER_RESP))
{
    cm_pin_request *req = malloc(sizeof(cm_pin_request));
    req->server = server;
    req->callback = callback;
    pthread_t pair_thread;
    pthread_create(&pair_thread, NULL, (void *(*)(void *))_pcmanager_unpairing_action, req);
    return true;
}

bool pcmanager_manual_add(const char *address, void (*callback)(PPCMANAGER_RESP))
{
    cm_pin_request *req = malloc(sizeof(cm_pin_request));
    req->arg1 = address;
    req->callback = callback;
    pthread_t add_thread;
    pthread_create(&add_thread, NULL, (void *(*)(void *))_manual_adding_action, (void *)req);
    return true;
}

void *_pcmanager_pairing_action(cm_pin_request *req)
{
    PPCMANAGER_RESP resp = serverinfo_resp_new();
    PSERVER_DATA server = serverdata_new();
    memcpy(server, req->server, sizeof(SERVER_DATA));
    int ret = gs_pair(server, (char *)req->arg1);
    if (ret != GS_OK)
    {
        pcmanager_resp_setgserror(resp, ret, gs_error);
        serverstate_setgserror(&resp->state, ret, gs_error);
    }
    resp->server = server;
    resp->server_shallow = true;
    bus_pushaction((bus_actionfunc)req->callback, resp);
    bus_pushaction((bus_actionfunc)handle_server_updated, resp);
    bus_pushaction((bus_actionfunc)serverinfo_resp_free, resp);
    free(req);
    return NULL;
}

void *_pcmanager_unpairing_action(cm_pin_request *req)
{
    PPCMANAGER_RESP resp = serverinfo_resp_new();
    PSERVER_DATA server = serverdata_new();
    memcpy(server, req->server, sizeof(SERVER_DATA));
    int ret = gs_unpair(server);
    if (ret != GS_OK)
        pcmanager_resp_setgserror(resp, ret, gs_error);
    resp->server = server;
    resp->server_shallow = true;
    bus_pushaction((bus_actionfunc)req->callback, resp);
    bus_pushaction((bus_actionfunc)handle_server_updated, resp);
    bus_pushaction((bus_actionfunc)serverinfo_resp_free, resp);
    free(req);
    return NULL;
}

void *_manual_adding_action(cm_pin_request *req)
{
    pcmanager_insert_by_address((const char *)req->arg1, true, req->callback);
    free(req);
    return NULL;
}

static int pin_random(int min, int max)
{
    return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}
