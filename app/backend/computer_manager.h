#pragma once

#include <stdbool.h>

#include "libgamestream/client.h"

typedef struct SERVER_LIST_T
{
    char *address;
    char *name;
    PSERVER_DATA server;
    int err;
    const char *errmsg;
    PAPP_LIST apps;
    struct SERVER_LIST_T *next;
} SERVER_LIST, *PSERVER_LIST;

extern PSERVER_LIST computer_list;

extern bool computer_manager_executing_quitapp;
extern bool computer_discovery_running;

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
 * @brief Starts discovery, and computer status update in background
 * 
 */
bool computer_manager_polling_start();

/**
 * @brief Stops polling
 * 
 */
void computer_manager_polling_stop();

PSERVER_LIST computer_manager_server_of(const char *address);

PSERVER_LIST computer_manager_server_at(int index);

typedef void (*pairing_callback)(PSERVER_LIST node, int result, const char *error);

/**
 * @brief Generates a PIN code, and start pairing process.
 * Generated PIN code will be written into `pin` pointer.
 * 
 * @param p 
 * @param pin 
 * @param cb Callback for pairing completion
 */
bool computer_manager_pair(PSERVER_LIST node, char *pin, pairing_callback cb);

bool computer_manager_quitapp(PSERVER_LIST node);

void *_computer_manager_polling_action(void *data);