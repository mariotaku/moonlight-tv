#pragma once

#include "../pcmanager.h"
#include "SDL_mutex.h"

#ifdef PCMANAGER_IMPL
#define LINKEDLIST_IMPL
#endif

#define LINKEDLIST_TYPE pcmanager_listener
#define LINKEDLIST_PREFIX pcmanager_callbacks
#define LINKEDLIST_DOUBLE 1

#include "util/linked_list.h"

#undef LINKEDLIST_DOUBLE
#undef LINKEDLIST_TYPE
#undef LINKEDLIST_PREFIX

#define LINKEDLIST_TYPE SERVER_LIST
#define LINKEDLIST_PREFIX serverlist
#define LINKEDLIST_DOUBLE 1

#include "util/linked_list.h"

#undef LINKEDLIST_DOUBLE
#undef LINKEDLIST_TYPE
#undef LINKEDLIST_PREFIX

struct pcmanager_t {
    SERVER_LIST *servers;
    SDL_mutex *servers_lock;
};

typedef struct CM_PIN_REQUEST_T {
    const SERVER_DATA *server;
    const void *arg1;
    void *userdata;

    pcmanager_callback_t callback;
} cm_request_t;

typedef struct {
    pcmanager_callback_t callback;
    PPCMANAGER_RESP resp;
    void *userdata;
} invoke_callback_t;

int pcmanager_update_sync(SERVER_LIST *existing, pcmanager_callback_t callback, void *userdata);

int pcmanager_insert_sync(const char *address, pcmanager_callback_t callback, void *userdata);

void serverdata_free(PSERVER_DATA data);

void serverlist_nodefree(PSERVER_LIST node);

void serverstate_setgserror(SERVER_STATE *state, int code, const char *msg);

void pcmanager_resp_setgserror(PPCMANAGER_RESP resp, int code, const char *msg);

PSERVER_DATA serverdata_new();

PPCMANAGER_RESP serverinfo_resp_new();

void serverinfo_resp_free(PPCMANAGER_RESP resp);

void serverlist_set_from_resp(PSERVER_LIST node, PPCMANAGER_RESP resp);

void handle_server_queried(PPCMANAGER_RESP resp);

void handle_server_updated(PPCMANAGER_RESP update);

void invoke_callback(invoke_callback_t *args);

invoke_callback_t *invoke_callback_args(PPCMANAGER_RESP resp, pcmanager_callback_t callback, void *userdata);

PSERVER_LIST pcmanager_find_by_address(const char *srvaddr);

void pcmanager_load_known_hosts(pcmanager_t *manager);

void pcmanager_save_known_hosts(pcmanager_t *manager);

int serverlist_compare_uuid(PSERVER_LIST other, const void *v);

void pcmanager_list_lock(pcmanager_t *manager);

void pcmanager_list_unlock(pcmanager_t *manager);