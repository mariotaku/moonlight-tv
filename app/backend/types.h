#pragma once

#include "libgamestream/client.h"

typedef enum SERVER_STATE_ENUM {
    SERVER_STATE_NONE = 0x0,
    SERVER_STATE_QUERYING = 0x10,
    SERVER_STATE_ONLINE = 0x20,
    SERVER_STATE_AVAILABLE = SERVER_STATE_ONLINE | 0x01,
    SERVER_STATE_NOT_PAIRED = SERVER_STATE_ONLINE | 0x02,
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

typedef struct appid_list_t {
    int id;
    struct appid_list_t *prev;
    struct appid_list_t *next;
} appid_list_t;

typedef struct SERVER_LIST_T {
    bool known, selected;
    SERVER_STATE state;
    /* DO NOT HOLD reference to this field*/
    const SERVER_DATA *server;
    appid_list_t *favs;
    struct SERVER_LIST_T *prev;
    struct SERVER_LIST_T *next;
} SERVER_LIST, *PSERVER_LIST;

