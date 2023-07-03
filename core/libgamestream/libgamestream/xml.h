/*
 * This file is part of Moonlight Embedded.
 *
 * Copyright (C) 2015 Iwan Timmer
 *
 * Moonlight is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Moonlight is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Moonlight; if not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include <stdio.h>
#include <stdbool.h>

typedef struct APP_LIST {
    char *name;
    int id;
    int hdr;
    struct APP_LIST *next;
} APP_LIST, *PAPP_LIST;

typedef struct DISPLAY_MODE {
    unsigned int height;
    unsigned int width;
    unsigned int refresh;
    struct DISPLAY_MODE *next;
} DISPLAY_MODE, *PDISPLAY_MODE;

int xml_search(char *data, size_t len, const char *node, char **result);

int xml_search_ex(char *data, size_t len, const char *node, bool required, char **result);

int xml_applist(char *data, size_t len, PAPP_LIST *app_list);

int xml_modelist(char *data, size_t len, PDISPLAY_MODE *mode_list);

int xml_status(char *data, size_t len);
