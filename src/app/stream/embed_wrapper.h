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

#include "version_info.h"

typedef struct embed_process_t embed_process_t;

int embed_check_version(version_info_t *version_info);

embed_process_t *embed_spawn(const char *app_name, const char *key_dir, const char *server_address,
                             int width, int height, int fps, int bitrate, bool localaudio, bool viewonly);

char *embed_read(embed_process_t *proc, char *buf, size_t size);

int embed_interrupt(embed_process_t *proc);

int embed_wait(embed_process_t *proc);