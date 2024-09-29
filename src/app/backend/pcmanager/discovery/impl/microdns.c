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
#include "impl.h"

#include <microdns/microdns.h>
#include "sockaddr.h"
#include "logging.h"

static void discovery_callback(discovery_task_t *task, int status, const struct rr_entry *entries);

static bool discovery_is_stopped(discovery_task_t *task);

int discovery_worker(discovery_task_t *task) {
    int r;
    char err[128];
    static const char *const service_name[] = {"_nvstream._tcp.local"};

    struct mdns_ctx *ctx = NULL;
    if ((r = mdns_init(&ctx, NULL, MDNS_PORT)) < 0) {
        goto err;
    }
    commons_log_info("Discovery", "Start mDNS discovery");
    if ((r = mdns_listen(ctx, service_name, 1, RR_PTR, 10, (mdns_stop_func) discovery_is_stopped,
                         (mdns_listen_callback) discovery_callback, task)) < 0) {
        goto err;
    }
    err:
    if (r < 0) {
        mdns_strerror(r, err, sizeof(err));
        commons_log_error("Discovery", "fatal: %s", err);
    }
    if (ctx != NULL) {
        mdns_destroy(ctx);
    }
    commons_log_info("Discovery", "mDNS discovery stopped");
    return r;
}

void discovery_worker_stop(discovery_task_t *task) {
    SDL_LockMutex(task->lock);
    task->stop = true;
    SDL_UnlockMutex(task->lock);
}

void discovery_callback(discovery_task_t *task, int status, const struct rr_entry *entries) {
    char err[128];

    if (status < 0) {
        mdns_strerror(status, err, sizeof(err));
        commons_log_error("Discovery", "error: %s", err);
        return;
    }
    if (task->stop) { return; }
    struct sockaddr *addr = sockaddr_new();
    for (const struct rr_entry *cur = entries; cur != NULL; cur = cur->next) {
        switch (cur->type) {
            case RR_A: {
                if (addr->sa_family != AF_UNSPEC) { continue; }
                sockaddr_set_ip(addr, AF_INET, &cur->data.A.addr);
                break;
            }
            case RR_AAAA: {
                if (addr->sa_family != AF_UNSPEC) { continue; }
                // Ignore any link-local addresses
                if (IN6_IS_ADDR_LINKLOCAL(&cur->data.AAAA.addr)) { continue; }
//                sockaddr_set_ip(addr, AF_INET6, &cur->data.AAAA.addr);
                break;
            }
            case RR_SRV: {
                sockaddr_set_port(addr, cur->data.SRV.port);
                break;
            }
        }
    }
    if (addr->sa_family == AF_UNSPEC) {
        return;
    }
    discovery_discovered(task->discovery, addr);
}

bool discovery_is_stopped(discovery_task_t *task) {
    SDL_LockMutex(task->lock);
    bool stop = task->stop;
    SDL_UnlockMutex(task->lock);
    return stop;
}