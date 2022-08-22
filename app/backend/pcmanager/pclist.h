/**
 * @file pclist.h
 *
 * Thread safe list management
 */
#pragma once

#include "../pcmanager.h"

pclist_t *pclist_insert_known(pcmanager_t *manager, const uuidstr_t *uuid, SERVER_DATA *server);

/**
 * Update item in server list if exists. Otherwise insert.
 * @param manager
 * @param server
 */
void pclist_upsert(pcmanager_t *manager, const uuidstr_t *uuid, const SERVER_STATE *state, SERVER_DATA *server);

void pclist_node_apply(pclist_t *node, const SERVER_STATE *state, SERVER_DATA *server);

bool pclist_node_set_app_favorite(pclist_t *node, int appid, bool favorite);

void pclist_remove(pcmanager_t *manager, const uuidstr_t *uuid);

void pclist_free(pcmanager_t *manager);

void pclist_lock(pcmanager_t *manager);

void pclist_unlock(pcmanager_t *manager);

pclist_t *pclist_find_by_uuid(pcmanager_t *manager, const uuidstr_t *uuid);

pclist_t *pclist_find_by_ip(pcmanager_t *manager, const char *ip);
