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

#include "throttle.h"
#include <time.h>
#include <SDL2/SDL_timer.h>

struct discovery_throttle_host_t {
    sockaddr_t *addr;
    Uint32 ttl, last_discovered;
    discovery_throttle_host_t *next;
    discovery_throttle_host_t *prev;
};

#define LINKEDLIST_IMPL
#define LINKEDLIST_MODIFIER static
#define LINKEDLIST_TYPE discovery_throttle_host_t
#define LINKEDLIST_PREFIX throttle_hosts
#define LINKEDLIST_DOUBLE 1

#include "linked_list.h"

#undef LINKEDLIST_DOUBLE
#undef LINKEDLIST_TYPE
#undef LINKEDLIST_PREFIX

static int throttle_hosts_compare_time(discovery_throttle_host_t *a, discovery_throttle_host_t *b);

static int throttle_hosts_find_addr(discovery_throttle_host_t *node, const void *addr);

static int throttle_hosts_find_not_expired(discovery_throttle_host_t *node, const void *now);

static void throttle_hosts_evict(discovery_throttle_host_t **head);

void discovery_throttle_init(discovery_throttle_t *throttle, discovery_callback callback, void *user_data) {
    throttle->callback = callback;
    throttle->user_data = user_data;
    throttle->hosts = NULL;
    throttle->lock = SDL_CreateMutex();
}

void discovery_throttle_deinit(discovery_throttle_t *throttle) {
    SDL_LockMutex(throttle->lock);
    throttle_hosts_free(throttle->hosts, NULL);
    SDL_UnlockMutex(throttle->lock);
    SDL_DestroyMutex(throttle->lock);
}

void discovery_throttle_on_discovered(discovery_throttle_t *throttle, const sockaddr_t *addr, Uint32 ttl) {
    // Remove all expired hosts
    SDL_LockMutex(throttle->lock);
    throttle_hosts_evict(&throttle->hosts);

    // Find existing host
    discovery_throttle_host_t *find = throttle_hosts_find_by(throttle->hosts, addr, throttle_hosts_find_addr);

    if (find != NULL) {
        // Ignore existing host
        SDL_UnlockMutex(throttle->lock);
        return;
    }

    discovery_throttle_host_t *node = throttle_hosts_new();
    node->addr = sockaddr_clone(addr);
    node->ttl = ttl;
    node->last_discovered = SDL_GetTicks();
    throttle->hosts = throttle_hosts_sortedinsert(throttle->hosts, node, throttle_hosts_compare_time);

    if (throttle->callback != NULL) {
        throttle->callback(addr, throttle->user_data);
    }
    SDL_UnlockMutex(throttle->lock);
}

int throttle_hosts_compare_time(discovery_throttle_host_t *a, discovery_throttle_host_t *b) {
    return (int) a->last_discovered - (int) b->last_discovered;
}

static int throttle_hosts_find_addr(discovery_throttle_host_t *node, const void *addr) {
    return sockaddr_compare(node->addr, addr);
}

void throttle_hosts_evict(discovery_throttle_host_t **head) {
    Uint32 now = SDL_GetTicks();
    discovery_throttle_host_t *find = throttle_hosts_find_by(*head, &now, throttle_hosts_find_not_expired);
    if (find != NULL) {
        if (find->prev != NULL) {
            find->prev->next = NULL;
        }
        find->prev = NULL;
        return;
    }
    throttle_hosts_free(*head, NULL);
    *head = find;
}

int throttle_hosts_find_not_expired(discovery_throttle_host_t *node, const void *now) {
    return SDL_TICKS_PASSED(*(Uint32 *) now, node->last_discovered + node->ttl);
}
