#pragma once

#include <glib.h>

#include "nvcomputer.h"

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
void computer_manager_polling_start();

/**
 * @brief Stops polling
 * 
 */
void computer_manager_polling_stop();

GList *computer_manager_list();

void _computer_manager_add(NVCOMPUTER *item);

gpointer _computer_manager_polling_action(gpointer data);