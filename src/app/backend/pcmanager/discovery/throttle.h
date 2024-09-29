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

#include "discovery.h"

void discovery_throttle_init(discovery_throttle_t *throttle, discovery_callback callback, void *user_data);

void discovery_throttle_deinit(discovery_throttle_t *throttle);

void discovery_throttle_on_discovered(discovery_throttle_t *throttle, const sockaddr_t *addr, Uint32 ttl);
