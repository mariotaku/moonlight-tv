#pragma once

#include <stdbool.h>

#include "backend/types.h"

#include "libgamestream/client.h"
#include "util/uuidstr.h"

typedef struct pcmanager_t pcmanager_t;

typedef struct PCMANAGER_RESP_T {
    union {
        int code;
        struct {
            int code;
            const char *message;
        } error;
    } result;
    bool known;
    SERVER_STATE state;
    SERVER_DATA *server;
    uuidstr_t uuid;
} pcmanager_resp_t, *PPCMANAGER_RESP;

typedef void (*pcmanager_callback_t)(const pcmanager_resp_t *, void *);

typedef struct pcmanager_listener_t {
    void (*added)(const pcmanager_resp_t *, void *userdata);

    void (*updated)(const pcmanager_resp_t *, void *userdata);

    void (*removed)(const pcmanager_resp_t *, void *userdata);
} pcmanager_listener_t;

/**
 * @brief Initialize computer manager context
 * 
 */
pcmanager_t *pcmanager_new();

/**
 * @brief Free all allocated memories, such as computer_list.
 * 
 */
void pcmanager_destroy(pcmanager_t *manager);

void pcmanager_auto_discovery_start(pcmanager_t *manager);

void pcmanager_auto_discovery_stop(pcmanager_t *manager);

PSERVER_LIST pcmanager_servers(pcmanager_t *manager);

bool pcmanager_manual_add(pcmanager_t *manager, const char *address, pcmanager_callback_t callback, void *userdata);

/**
 * @brief Generates a PIN code, and start pairing process.
 * Generated PIN code will be written into `pin` pointer.
 * 
 * @param p 
 * @param pin 
 */
bool pcmanager_pair(pcmanager_t *manager, const uuidstr_t *uuid, char *pin, pcmanager_callback_t callback,
                    void *userdata);

bool pcmanager_quitapp(pcmanager_t *manager, const uuidstr_t *uuid, pcmanager_callback_t callback, void *userdata);

void pcmanager_request_update(pcmanager_t *manager, const uuidstr_t *uuid, pcmanager_callback_t callback,
                              void *userdata);

bool pcmanager_send_wol(pcmanager_t *manager, const uuidstr_t *uuid, pcmanager_callback_t callback,
                        void *userdata);

void pcmanager_favorite_app(pcmanager_t *manager, const uuidstr_t *uuid, int appid, bool favorite);

bool pcmanager_is_favorite(pcmanager_t *manager, const uuidstr_t *uuid, int appid);

const SERVER_LIST *pcmanager_node(pcmanager_t *manager, const uuidstr_t *uuid);

const SERVER_LIST *pcmanager_select(pcmanager_t *manager, const uuidstr_t *uuid);

const SERVER_STATE *pcmanager_state(pcmanager_t *manager, const uuidstr_t *uuid);

int pcmanager_server_current_app(pcmanager_t *manager, const uuidstr_t *uuid);

bool pcmanager_node_is_app_favorite(const SERVER_LIST *node, int appid);

void pcmanager_node_set_app_favorite(SERVER_LIST *node, int appid, bool favorite);

void pcmanager_register_listener(pcmanager_t *manager, const pcmanager_listener_t *listener, void *userdata);

void pcmanager_unregister_listener(pcmanager_t *manager, const pcmanager_listener_t *listener);
