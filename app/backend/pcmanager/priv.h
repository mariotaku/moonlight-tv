#pragma once
#include "backend/pcmanager.h"

typedef struct CM_PIN_REQUEST_T
{
    const SERVER_DATA *server;
    const void *arg1;
    void (*callback)(PPCMANAGER_RESP);
} cm_pin_request;

int pcmanager_insert_by_address(const char *srvaddr, bool pair, void (*callback)(PPCMANAGER_RESP));

void serverdata_free(PSERVER_DATA data);

void serverlist_nodefree(PSERVER_LIST node);

void serverstate_setgserror(SERVER_STATE *state, int code, const char *msg);

void pcmanager_resp_setgserror(PPCMANAGER_RESP resp, int code, const char *msg);

void *_computer_manager_polling_action(void *data);

PSERVER_DATA serverdata_new();

PPCMANAGER_RESP serverinfo_resp_new();

void serverinfo_resp_free(PPCMANAGER_RESP resp);

void handle_server_updated(PPCMANAGER_RESP update);