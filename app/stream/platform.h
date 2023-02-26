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

#include "config.h"

#include <Limelight.h>

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>

#include "os_info.h"

typedef struct DECODER_INFO {
    /* Decoder passes the check */
    bool valid;
    /* Decoder supports hardware acceleration */
    bool accelerated;
    /* Decoder has built-in audio feature */
    bool audio;
    /* Decoder supports HEVC video stream */
    bool hevc;
    /* Decoder supports HDR */
    int hdr;
    int colorSpace;
    int colorRange;
    int maxBitrate;
    int maxFramerate;
    int audioConfig;
    /* Handles video renderer on screen */
    bool hasRenderer;
    int suggestedBitrate;
    bool canResize;
} DECODER_INFO;

typedef struct audio_config_entry_t {
    int configuration;
    const char *value;
    const char *name;
} audio_config_entry_t;

extern DECODER_INFO decoder_info;

extern const audio_config_entry_t audio_configs[];
extern const size_t audio_config_len;

bool decoder_max_dimension(int *width, int *height);

int decoder_max_framerate();

int module_audio_configuration();

const char *module_geterror();

