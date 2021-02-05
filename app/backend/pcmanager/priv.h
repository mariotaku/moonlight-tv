#pragma once
#include "backend/computer_manager.h"

typedef struct CM_PIN_REQUEST_T
{
    PSERVER_LIST node;
    char *pin;
} cm_pin_request;

int pcmanager_insert_by_address(char *srvaddr, bool pair);