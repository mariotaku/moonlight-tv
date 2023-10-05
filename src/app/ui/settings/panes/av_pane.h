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

#include "app.h"
#include "lvgl.h"
#include "pref_obj.h"

void update_conflict_hint(app_t *app, lv_obj_t *hint);

bool contains_decoder_group(const pref_dropdown_string_entry_t *entry, size_t len, const char *group);

void set_decoder_entry(pref_dropdown_string_entry_t *entry, const char *name, const char *group, bool fallback);