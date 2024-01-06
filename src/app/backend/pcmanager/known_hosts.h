/*
 * Copyright (c) 2023 Ningyuan Li <https://github.com/mariotaku>.
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

#include "backend/types.h"
#include "uuidstr.h"
#include "sockaddr.h"

typedef struct known_host_t {
    uuidstr_t uuid;
    char *mac;
    char *hostname;
    struct sockaddr *address;
    bool selected;
    appid_list_t *favs;
    appid_list_t *hidden;
    struct known_host_t *next;
} known_host_t;

void known_hosts_node_free(known_host_t *node);

known_host_t *known_hosts_parse(const char *conf_file);

#define LINKEDLIST_IMPL
#define LINKEDLIST_MODIFIER static
#define LINKEDLIST_TYPE known_host_t
#define LINKEDLIST_PREFIX known_hosts

#include "linked_list.h"

#undef LINKEDLIST_TYPE
#undef LINKEDLIST_PREFIX