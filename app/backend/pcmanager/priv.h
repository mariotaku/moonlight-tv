#pragma once
#include "backend/computer_manager.h"

typedef struct CM_PIN_REQUEST_T
{
    PSERVER_LIST node;
    char *pin;
    void (*callback)(PSERVER_LIST);
} cm_pin_request;

int pcmanager_insert_by_address(const char *srvaddr, bool pair);

void serverdata_free(PSERVER_DATA data);
void serverlist_nodefree(PSERVER_LIST node);

void serverstate_setgserror(SERVER_STATE *state, int code, const char *msg);