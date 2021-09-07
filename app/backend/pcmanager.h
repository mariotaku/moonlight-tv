#pragma once

#include <stdbool.h>

#include "backend/types.h"

#include "libgamestream/client.h"

extern PSERVER_LIST computer_list;
extern bool computer_discovery_running;
extern bool pcmanager_setup_running;

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
    bool server_shallow;
    bool server_referenced;
} PCMANAGER_RESP, *PPCMANAGER_RESP;

typedef void (*pcmanager_callback_t)(PPCMANAGER_RESP, void *);

typedef struct PCMANAGER_CALLBACKS {
    void (*added)(void *userdata, PPCMANAGER_RESP);

    void (*updated)(void *userdata, PPCMANAGER_RESP);

    void *userdata;

    struct PCMANAGER_CALLBACKS *prev;
    struct PCMANAGER_CALLBACKS *next;
} PCMANAGER_CALLBACKS, *PPCMANAGER_CALLBACKS;

/**
 * @brief Initialize computer manager context
 * 
 */
void computer_manager_init();

/**
 * @brief Free all allocated memories, such as computer_list.
 * 
 */
void computer_manager_destroy();

bool computer_manager_dispatch_userevent(int which, void *data1, void *data2);

/**
 * @brief Run scan task
 * 
 */
bool computer_manager_run_scan();

void computer_manager_auto_discovery_schedule(unsigned int ms);

void pcmanager_auto_discovery_start();

void pcmanager_auto_discovery_stop();

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

bool pcmanager_manual_add(const char *address, pcmanager_callback_t callback, void *userdata);

bool pcmanager_send_wol(const SERVER_DATA *server);

void pcmanager_request_update(const SERVER_DATA *server);

void pcmanager_register_callbacks(PPCMANAGER_CALLBACKS callbacks);

void pcmanager_unregister_callbacks(PPCMANAGER_CALLBACKS callbacks);
