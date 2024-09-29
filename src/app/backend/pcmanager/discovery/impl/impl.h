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

typedef struct discovery_task_t {
    struct discovery_t *discovery;
    SDL_mutex *lock;
    bool stop;
} discovery_task_t;

int discovery_worker(discovery_task_t *task);

void discovery_worker_stop(discovery_task_t *task);

void discovery_discovered(struct discovery_t *discovery, const sockaddr_t *addr);