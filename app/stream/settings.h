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

#include <Limelight.h>

#include <stdbool.h>
#include "ss4s/video.h"

typedef struct window_state_t {
    int x, y, w, h;
} window_state_t;

typedef struct configuration_t {
    STREAM_CONFIGURATION stream;
    int debug_level;
    char *decoder;
    char *audio_backend;
    char *audio_device;
    char *language;
    char key_dir[4096];
    bool sops;
    bool localaudio;
    bool fullscreen;
    window_state_t window_state;
    int rotate;
    bool unsupported;
    bool quitappafter;
    bool viewonly;
    bool absmouse;
    bool hardware_mouse;
    bool virtual_mouse;
    bool swap_abxy;
    bool syskey_capture;
    bool stop_on_stall;
    bool hdr;
    bool hevc;

    /*Volatile fields*/
    char default_host_uuid[40];
    int default_app_id;
} CONFIGURATION, *PCONFIGURATION;

#define CONF_NAME_MOONLIGHT "moonlight.ini"
#define CONF_NAME_HOSTS "hosts.ini"

#define RES_MERGE(w, h) (((w) & 0xFFFF) << 16 | ((h) & 0xFFFF))

#define RES_720P RES_MERGE(1280, 720)
#define RES_1080P RES_MERGE(1920, 1080)
#define RES_1440P RES_MERGE(2560, 1440)
#define RES_4K RES_MERGE(3840, 2160)

PCONFIGURATION settings_load();

void settings_save(PCONFIGURATION config);

void settings_free(PCONFIGURATION config);

int settings_optimal_bitrate(const SS4S_VideoCapabilities *capabilities,int w, int h, int fps);

bool audio_config_valid(int config);