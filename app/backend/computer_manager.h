#pragma once

#include <stdbool.h>

#include "backend/types.h"

#include "libgamestream/client.h"

extern PSERVER_LIST computer_list;
extern bool computer_discovery_running;
extern bool pcmanager_setup_running;

typedef struct PCMANAGER_RESP_T
{
  union
  {
    int code;
    struct
    {
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

typedef struct PCMANAGER_CALLBACKS
{
  void (*onAdded)(PPCMANAGER_RESP);
  void (*onUpdated)(PPCMANAGER_RESP);
  struct PCMANAGER_CALLBACKS *prev;
  struct PCMANAGER_CALLBACKS *next;
} PCMANAGER_CALLBACKS, *PPCMANAGER_CALLBACKS;

#ifdef PCMANAGER_IMPL
#define LINKEDLIST_IMPL
#endif

#define LINKEDLIST_TYPE PCMANAGER_CALLBACKS
#define LINKEDLIST_PREFIX pcmanager_callbacks
#define LINKEDLIST_DOUBLE 1
#include "util/linked_list.h"

#undef LINKEDLIST_DOUBLE
#undef LINKEDLIST_TYPE
#undef LINKEDLIST_PREFIX

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
bool pcmanager_pair(const SERVER_DATA *server, char *pin, void (*callback)(PPCMANAGER_RESP));

bool pcmanager_unpair(const SERVER_DATA *server, void (*callback)(PPCMANAGER_RESP));

bool pcmanager_quitapp(const SERVER_DATA *server, void (*callback)(PPCMANAGER_RESP));

bool pcmanager_manual_add(const char *address, void (*callback)(PPCMANAGER_RESP));

bool pcmanager_send_wol(const SERVER_DATA *server);

void pcmanager_request_update(const SERVER_DATA *server);

void pcmanager_register_callbacks(PPCMANAGER_CALLBACKS callbacks);
void pcmanager_unregister_callbacks(PPCMANAGER_CALLBACKS callbacks);