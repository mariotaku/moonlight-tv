#pragma once

#include <glib.h>

#include "libgamestream/client.h"

/**
 * @brief Initialize application manager context
 * 
 */
void application_manager_init();

/**
 * @brief Free all allocated memories
 * 
 */
void application_manager_destroy();

/**
 * @brief Load applications list for server in background
 * 
 * @param server 
 */
void application_manager_load(SERVER_DATA *server);