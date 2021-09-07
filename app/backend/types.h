#pragma once

#include "libgamestream/client.h"

typedef enum SERVER_STATE_ENUM {
    SERVER_STATE_NONE,
    SERVER_STATE_ONLINE,
    SERVER_STATE_OFFLINE,
    SERVER_STATE_ERROR,
} SERVER_STATE_ENUM;

typedef union SERVER_STATE {
    SERVER_STATE_ENUM code;
    struct {
        SERVER_STATE_ENUM code;
    } offline;
    struct {
        SERVER_STATE_ENUM code;
        int errcode;
        const char *errmsg;
    } error;
} SERVER_STATE;

typedef struct SERVER_LIST_T {
    bool known, selected;
    SERVER_STATE state;
    const SERVER_DATA *server;
    struct SERVER_LIST_T *prev;
    struct SERVER_LIST_T *next;
} SERVER_LIST, *PSERVER_LIST;

