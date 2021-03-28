#pragma once

#include <stdbool.h>

#include "backend_types.h"

#include "libgamestream/client.h"

PSERVER_LIST computer_list;

bool computer_manager_executing_quitapp;
bool computer_discovery_running;

typedef struct SERVER_INFO_RESP_T
{
  bool known;
  SERVER_STATE state;
  const SERVER_DATA *server;
} SERVER_INFO_RESP, *PSERVER_INFO_RESP;

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

void computer_manager_auto_discovery_start();

void computer_manager_auto_discovery_stop();

PSERVER_LIST computer_manager_server_of(const char *address);

PSERVER_LIST computer_manager_server_at(int index);

/**
 * @brief Generates a PIN code, and start pairing process.
 * Generated PIN code will be written into `pin` pointer.
 * 
 * @param p 
 * @param pin 
 */
bool computer_manager_pair(PSERVER_LIST node, char *pin, void (*callback)(PSERVER_LIST));

bool computer_manager_unpair(PSERVER_LIST node, void (*callback)(PSERVER_LIST));

bool computer_manager_quitapp(PSERVER_LIST node);

bool pcmanager_send_wol(PSERVER_LIST node);

bool pcmanager_manual_add(const char *address);

void *_computer_manager_polling_action(void *data);

PSERVER_DATA serverdata_new();

PSERVER_INFO_RESP serverinfo_resp_new();