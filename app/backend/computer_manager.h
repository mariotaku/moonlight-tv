#pragma once

#include <stdbool.h>

#include "libgamestream/client.h"

typedef struct SERVER_LIST_T
{
    char *name;
    PSERVER_DATA server;
    int err;
    const char *errmsg;
    PAPP_LIST apps;
    struct SERVER_LIST_T *next;
} SERVER_LIST, *PSERVER_LIST;

extern PSERVER_LIST computer_list;

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

typedef void (*pairing_callback)(int result, const char *error);

/**
 * @brief Generates a PIN code, and start pairing process.
 * Generated PIN code will be written into `pin` pointer.
 * 
 * @param p 
 * @param pin 
 * @param cb Callback for pairing completion
 */
bool computer_manager_pair(PSERVER_DATA p, char *pin, pairing_callback cb);

void _computer_manager_add(char *name, PSERVER_DATA p, int err);

int _computer_manager_polling_action(void *data);