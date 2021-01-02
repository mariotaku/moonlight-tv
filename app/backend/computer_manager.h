#pragma once

#include <stdbool.h>
#include <glib.h>

#include "libgamestream/client.h"
#include "libgamestream/../src/config.h"

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

GList *computer_manager_list();

typedef void (*pairing_callback)(int result, const char *error);

/**
 * @brief Generates a PIN code, and start pairing process.
 * Generated PIN code will be written into `pin` pointer.
 * 
 * @param p 
 * @param pin 
 * @param cb Callback for pairing completion
 */
bool computer_manager_pair(SERVER_DATA *p, char *pin, pairing_callback cb);

void _computer_manager_add(SERVER_DATA *item);

gpointer _computer_manager_polling_action(gpointer data);