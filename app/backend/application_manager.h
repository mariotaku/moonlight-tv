#pragma once

#include "libgamestream/client.h"

#include "backend/types.h"

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
 * @param node 
 */
void application_manager_load(PSERVER_LIST node);

bool application_manager_dispatch_userevent(int which, void *data1, void *data2);