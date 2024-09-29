/*
 * Copyright (c) 2024 Mariotaku <https://github.com/mariotaku>.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include "priv.h"
#include "pclist.h"
#include "app.h"
#include "errors.h"

#include "logging.h"

static void lan_host_status_update(pcmanager_t *manager, SERVER_DATA *server);

static void lan_host_offline(pcmanager_t *manager, const sockaddr_t *addr);

void pcmanager_lan_host_discovered(const sockaddr_t *addr, pcmanager_t *manager) {
    GS_CLIENT client = app_gs_client_new(manager->app);
    SERVER_DATA *server = serverdata_new();
    char ip[64];
    sockaddr_get_ip_str(addr, ip, sizeof(ip));
    int ret = gs_get_status(client, server, strndup(ip, sizeof(ip)), sockaddr_get_port(addr),
                            app_configuration->unsupported);
    if (ret == GS_OK) {
        commons_log_info("PCManager", "Finished updating status from %s", ip);
        lan_host_status_update(manager, server);
    } else {
        serverdata_free(server);
        const char *gs_error = NULL;
        ret = gs_get_error(&gs_error);
        if (ret == GS_IO_ERROR) {
            commons_log_warn("PCManager", "Error while updating status from %s. Host seems to be offline", ip);
            lan_host_offline(manager, addr);
        } else {
            commons_log_warn("PCManager", "Error while updating status from %s: %d (%s)", ip, ret, gs_error);
        }
    }
    gs_destroy(client);
}


void lan_host_status_update(pcmanager_t *manager, SERVER_DATA *server) {
    SERVER_STATE state = {.code = server->paired ? SERVER_STATE_AVAILABLE : SERVER_STATE_NOT_PAIRED};
    pclist_upsert(manager, (const uuidstr_t *) server->uuid, &state, server);
}

void lan_host_offline(pcmanager_t *manager, const sockaddr_t *addr) {
    pclist_t *existing = pclist_find_by_addr(manager, addr);
    if (!existing) {
        return;
    }
    pcmanager_lock(manager);
    existing->state.code = SERVER_STATE_OFFLINE;
    pcmanager_unlock(manager);
}