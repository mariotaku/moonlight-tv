#pragma once
#include "backend/computer_manager.h"

typedef struct CM_PIN_REQUEST_T
{
    const SERVER_DATA *server;
    const char *pin;
    void (*callback)(PSERVER_INFO_RESP);
} cm_pin_request;

int pcmanager_insert_by_address(const char *srvaddr, bool pair);

void serverdata_free(PSERVER_DATA data);

void serverlist_nodefree(PSERVER_LIST node);

void serverstate_setgserror(SERVER_STATE *state, int code, const char *msg);

void *_computer_manager_polling_action(void *data);

PSERVER_DATA serverdata_new();

PSERVER_INFO_RESP serverinfo_resp_new();

void serverinfo_resp_free(PSERVER_INFO_RESP resp);

void handle_server_updated(PSERVER_INFO_RESP update);