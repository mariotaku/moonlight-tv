#pragma once

#include "../pcmanager.h"

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

int _computer_manager_polling_action(void *data);

PSERVER_DATA serverdata_new();

PPCMANAGER_RESP serverinfo_resp_new();

void serverinfo_resp_free(PPCMANAGER_RESP resp);

void handle_server_updated(PPCMANAGER_RESP update);

void invoke_callback(invoke_callback_t *args);

invoke_callback_t *invoke_callback_args(PPCMANAGER_RESP resp, pcmanager_callback_t callback, void *userdata);