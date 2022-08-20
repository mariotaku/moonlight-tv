/**
 * @file pclist.h
 *
 * Thread safe list management
 */
#pragma once

#include "../pcmanager.h"

SERVER_LIST *pclist_insert_known(pcmanager_t *manager, SERVER_DATA *server);

/**
 * Update item in server list if exists. Otherwise insert.
 * @param manager
 * @param server
 */
void pclist_upsert(pcmanager_t *manager, pcmanager_resp_t *resp);

void pclist_remove(pcmanager_t *manager, SERVER_DATA *server);

void pclist_free(pcmanager_t *manager);

void pclist_lock(pcmanager_t *manager);

void pclist_unlock(pcmanager_t *manager);
