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

#include "embed_wrapper.h"

#include <stdio.h>
#include <string.h>

int embed_check_version(version_info_t *version_info) {
    FILE *f = popen("moonlight help", "r");
    if (f == NULL) {
        return -1;
    }
    // Get first line of output
    char buf[64];
    if (fgets(buf, sizeof(buf), f) == NULL) {
        pclose(f);
        return -1;
    }
    buf[sizeof(buf) - 1] = '\0';
    int ret = pclose(f);
    if (ret != 0) {
        return ret;
    }
    if (strncmp(buf, "Moonlight Embedded ", 19) != 0) {
        return -1;
    }
    return version_info_parse(version_info, buf + 19);
}