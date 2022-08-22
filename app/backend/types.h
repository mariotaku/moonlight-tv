#pragma once

#include "libgamestream/client.h"
#include "util/uuidstr.h"

typedef enum SERVER_STATE_ENUM {
    /** Initial state */
    SERVER_STATE_NONE = 0x0,
    /** PCManager is requesting server info */
    SERVER_STATE_QUERYING = 0x10,
    /** Server is online */
    SERVER_STATE_ONLINE = 0x20,
    /** Server is online and paired */
    SERVER_STATE_AVAILABLE = SERVER_STATE_ONLINE | 0x01,
    /** Server is online but not paired */
    SERVER_STATE_NOT_PAIRED = SERVER_STATE_ONLINE | 0x02,
    /** Can't reach server */
    SERVER_STATE_OFFLINE = 0x30,
    /** Server returned error */
    SERVER_STATE_ERROR = 0x40,
} SERVER_STATE_ENUM;

typedef union SERVER_STATE {
    SERVER_STATE_ENUM code;
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

typedef struct pclist_t {
    uuidstr_t id;
    bool known, selected;
    SERVER_STATE state;
    /* DO NOT HOLD reference to this field*/
    SERVER_DATA *server;
    appid_list_t *favs;
    struct pclist_t *prev;
    struct pclist_t *next;
} pclist_t;

