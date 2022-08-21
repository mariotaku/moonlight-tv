#pragma once

#include <stdbool.h>

#include "backend/types.h"

#include "libgamestream/client.h"
#include "util/uuidstr.h"

typedef struct pcmanager_t pcmanager_t;

typedef void (*pcmanager_callback_t)(int result, const char *error, const uuidstr_t *uuid, void *userdata);

typedef void(*pcmanager_listener_fn)(const uuidstr_t *uuid, void *userdata);

typedef struct pcmanager_listener_t {
    pcmanager_listener_fn added;
    pcmanager_listener_fn updated;
    pcmanager_listener_fn removed;
} pcmanager_listener_t;

/**
 * @brief Initialize computer manager context
 * 
 */
pcmanager_t *pcmanager_new();

/**
 * @brief Free all allocated memories, such as computer_list.
 * 
 */
void pcmanager_destroy(pcmanager_t *manager);

void pcmanager_auto_discovery_start(pcmanager_t *manager);

void pcmanager_auto_discovery_stop(pcmanager_t *manager);

pclist_t *pcmanager_servers(pcmanager_t *manager);

bool pcmanager_manual_add(pcmanager_t *manager, const char *address, pcmanager_callback_t callback, void *userdata);

/**
 * @brief Generates a PIN code, and start pairing process.
 * Generated PIN code will be written into `pin` pointer.
 * 
 * @param p 
 * @param pin 
 */
bool pcmanager_pair(pcmanager_t *manager, const uuidstr_t *uuid, char *pin, pcmanager_callback_t callback,
                    void *userdata);

bool pcmanager_quitapp(pcmanager_t *manager, const uuidstr_t *uuid, pcmanager_callback_t callback, void *userdata);

void pcmanager_request_update(pcmanager_t *manager, const uuidstr_t *uuid, pcmanager_callback_t callback,
                              void *userdata);

bool pcmanager_send_wol(pcmanager_t *manager, const uuidstr_t *uuid, pcmanager_callback_t callback,
                        void *userdata);

void pcmanager_favorite_app(pcmanager_t *manager, const uuidstr_t *uuid, int appid, bool favorite);

bool pcmanager_is_favorite(pcmanager_t *manager, const uuidstr_t *uuid, int appid);

bool pcmanager_select(pcmanager_t *manager, const uuidstr_t *uuid);

bool pcmanager_forget(pcmanager_t *manager, const uuidstr_t *uuid);

const pclist_t *pcmanager_node(pcmanager_t *manager, const uuidstr_t *uuid);

const SERVER_STATE *pcmanager_state(pcmanager_t *manager, const uuidstr_t *uuid);

int pcmanager_server_current_app(pcmanager_t *manager, const uuidstr_t *uuid);

bool pcmanager_node_is_app_favorite(const pclist_t *node, int appid);

void pcmanager_node_set_app_favorite(pclist_t *node, int appid, bool favorite);

void pcmanager_register_listener(pcmanager_t *manager, const pcmanager_listener_t *listener, void *userdata);

void pcmanager_unregister_listener(pcmanager_t *manager, const pcmanager_listener_t *listener);

/**
 *
 * @param manager pcmanager instance
 * @param ip
 * @param force If false, update will be omitted if the server is already online
 * @param callback
 * @param userdata
 * @return
 */
int pcmanager_update_by_ip(pcmanager_t *manager, const char *ip, bool force);