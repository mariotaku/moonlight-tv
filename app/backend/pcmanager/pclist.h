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
void pclist_upsert(pcmanager_t *manager, const pcmanager_resp_t *resp);

void pclist_free(pcmanager_t *manager);