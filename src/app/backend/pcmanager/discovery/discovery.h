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

#pragma once

#include <stdbool.h>
#include <SDL2/SDL.h>
#include "sockaddr.h"

typedef void (*discovery_callback)(const sockaddr_t *addr, void *user_data);

typedef struct discovery_throttle_host_t discovery_throttle_host_t;

typedef struct discovery_throttle_t {
    discovery_callback callback;
    void *user_data;
    discovery_throttle_host_t *hosts;
    SDL_mutex *lock;
} discovery_throttle_t;

typedef struct discovery_t {
    SDL_mutex *lock;
    struct discovery_task_t *task;
    discovery_throttle_t throttle;
} discovery_t;

void discovery_init(discovery_t *discovery, discovery_callback callback, void *user_data);

void discovery_start(discovery_t *discovery);

void discovery_stop(discovery_t *discovery);

void discovery_deinit(discovery_t *discovery);
