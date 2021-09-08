#pragma once

#include "../pcmanager.h"
#include <SDL.h>

typedef struct pcmanager_listener_list pcmanager_listener_list;

struct pcmanager_t {
    SDL_threadID thread_id;
    SERVER_LIST *servers;
    SDL_mutex *servers_lock;
    pcmanager_listener_list *listeners;
};

typedef struct CM_PIN_REQUEST_T {
    const SERVER_DATA *server;
    const void *arg1;
    void *userdata;

    pcmanager_callback_t callback;
} cm_request_t;

typedef struct {
    pcmanager_callback_t callback;
    pcmanager_resp_t *resp;
    void *userdata;
} pcmanager_finalizer_args;

void serverdata_free(PSERVER_DATA data);

void serverstate_setgserror(SERVER_STATE *state, int code, const char *msg);

void pcmanager_resp_setgserror(PPCMANAGER_RESP resp, int code, const char *msg);

PSERVER_DATA serverdata_new();

PPCMANAGER_RESP serverinfo_resp_new();

void pclist_node_apply(PSERVER_LIST node, const pcmanager_resp_t *resp);

void pcmanager_worker_finalize(pcmanager_finalizer_args *args);

pcmanager_finalizer_args *pcmanager_finalize_args(pcmanager_resp_t *resp, pcmanager_callback_t callback,
                                                  void *userdata);

PSERVER_LIST pcmanager_find_by_address(pcmanager_t *manager, const char *srvaddr);

void pcmanager_load_known_hosts(pcmanager_t *manager);

void pcmanager_save_known_hosts(pcmanager_t *manager);

void pcmanager_list_lock(pcmanager_t *manager);

void pcmanager_list_unlock(pcmanager_t *manager);

int pcmanager_upsert_worker(pcmanager_t *manager, const char *address, pcmanager_callback_t callback, void *userdata);