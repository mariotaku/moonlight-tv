#pragma once

#include <stdbool.h>

#include "backend/types.h"

#include "libgamestream/client.h"

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
    const SERVER_DATA *server;
} pcmanager_resp_t, *PPCMANAGER_RESP;

typedef void (*pcmanager_callback_t)(const pcmanager_resp_t *, void *);

typedef struct pcmanager_listener_t {
    void (*added)(const pcmanager_resp_t *, void *userdata);

    void (*updated)(const pcmanager_resp_t *, void *userdata);
} pcmanager_listener_t;

/**
 * @brief Initialize computer manager context
 * 
 */
pcmanager_t *computer_manager_new();

/**
 * @brief Free all allocated memories, such as computer_list.
 * 
 */
void computer_manager_destroy(pcmanager_t *);

void pcmanager_auto_discovery_start(pcmanager_t *manager);

void pcmanager_auto_discovery_stop(pcmanager_t *manager);

PSERVER_LIST pcmanager_servers(pcmanager_t *manager);

/**
 * @brief Generates a PIN code, and start pairing process.
 * Generated PIN code will be written into `pin` pointer.
 * 
 * @param p 
 * @param pin 
 */
bool pcmanager_pair(const SERVER_DATA *server, char *pin, pcmanager_callback_t callback, void *userdata);

bool pcmanager_unpair(const SERVER_DATA *server, pcmanager_callback_t callback, void *userdata);

bool pcmanager_quitapp(const SERVER_DATA *server, pcmanager_callback_t callback, void *userdata);

bool pcmanager_send_wol(const SERVER_DATA *server);

void pcmanager_request_update(const SERVER_DATA *server);

bool pcmanager_manual_add(const char *address, pcmanager_callback_t callback, void *userdata);

void pcmanager_register_listener(pcmanager_t *manager, const pcmanager_listener_t *listener, void *userdata);

void pcmanager_unregister_listener(pcmanager_t *manager, const pcmanager_listener_t *listener);
