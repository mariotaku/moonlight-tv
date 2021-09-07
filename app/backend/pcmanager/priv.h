#pragma once

#include "../pcmanager.h"

#ifdef PCMANAGER_IMPL
#define LINKEDLIST_IMPL
#endif

#define LINKEDLIST_TYPE PCMANAGER_CALLBACKS
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

int pcmanager_insert_by_address(const char *srvaddr, bool pair, pcmanager_callback_t callback, void *userdata);

void serverdata_free(PSERVER_DATA data);

void serverlist_nodefree(PSERVER_LIST node);

void serverstate_setgserror(SERVER_STATE *state, int code, const char *msg);

void pcmanager_resp_setgserror(PPCMANAGER_RESP resp, int code, const char *msg);

PSERVER_DATA serverdata_new();

PPCMANAGER_RESP serverinfo_resp_new();

void serverinfo_resp_free(PPCMANAGER_RESP resp);

void handle_server_discovered(PPCMANAGER_RESP discovered);

void handle_server_updated(PPCMANAGER_RESP update);

void invoke_callback(invoke_callback_t *args);

invoke_callback_t *invoke_callback_args(PPCMANAGER_RESP resp, pcmanager_callback_t callback, void *userdata);

bool pcmanager_is_known_host(const char *srvaddr);

void pcmanager_load_known_hosts();

void pcmanager_save_known_hosts();

int serverlist_compare_uuid(PSERVER_LIST other, const void *v);
