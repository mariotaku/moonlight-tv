/*
 * This file is part of Moonlight Embedded.
 *
 * Copyright (C) 2015-2017 Iwan Timmer
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

#include "xml.h"

#include <Limelight.h>

#include <stdbool.h>

#define MIN_SUPPORTED_GFE_VERSION 3
#define MAX_SUPPORTED_GFE_VERSION 7

typedef struct _SERVER_DATA {
    const char *uuid;
    const char *mac;
    const char *hostname;
    const char *gpuType;
    bool paired;
    bool supports4K;
    bool supportsHdr;
    bool unsupported;
    int currentGame;
    int serverMajorVersion;
    const char *gsVersion;
    PDISPLAY_MODE modes;
    SERVER_INFORMATION serverInfo;
} SERVER_DATA, *PSERVER_DATA;

typedef struct GS_CLIENT_T *GS_CLIENT;

GS_CLIENT gs_new(const char *keydir, int log_level);

void gs_destroy(GS_CLIENT);

int gs_init(GS_CLIENT, PSERVER_DATA server, const char *address, bool unsupported);

int gs_start_app(GS_CLIENT, PSERVER_DATA server, PSTREAM_CONFIGURATION config, int appId, bool sops, bool localaudio,
                 int gamepad_mask);

int gs_applist(GS_CLIENT, const SERVER_DATA *server, PAPP_LIST *app_list);

int gs_unpair(GS_CLIENT, PSERVER_DATA server);

int gs_pair(GS_CLIENT, PSERVER_DATA server, const char *pin);

int gs_quit_app(GS_CLIENT, PSERVER_DATA server);

int gs_download_cover(GS_CLIENT, PSERVER_DATA server, int appId, const char *path);