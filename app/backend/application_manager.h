#pragma once

#include <SDL.h>

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

bool application_manager_dispatch_event(SDL_Event ev);

PAPP_LIST application_manager_list_of(const char *address);

int applist_len(PAPP_LIST p);

PAPP_LIST applist_nth(PAPP_LIST p, int n);