#pragma once

#include <stdbool.h>

#include "backend_types.h"

#include "libgamestream/client.h"

PSERVER_LIST computer_list;

bool computer_discovery_running;

typedef struct SERVER_INFO_RESP_T
{
  bool known;
  SERVER_STATE state;
  const SERVER_DATA *server;
  bool server_referenced;
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

/**
 * @brief Generates a PIN code, and start pairing process.
 * Generated PIN code will be written into `pin` pointer.
 * 
 * @param p 
 * @param pin 
 */
bool pcmanager_pair(const SERVER_DATA *server, char *pin, void (*callback)(PSERVER_INFO_RESP));

bool pcmanager_unpair(const SERVER_DATA *server, void (*callback)(PSERVER_INFO_RESP));

bool pcmanager_quitapp(const SERVER_DATA *server, void (*callback)(PSERVER_INFO_RESP));

bool pcmanager_send_wol(const SERVER_DATA *server);

void pcmanager_request_update(const SERVER_DATA *server);

bool pcmanager_manual_add(const char *address);