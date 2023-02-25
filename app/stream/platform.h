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

enum DECODER_T {
    DECODER_AUTO = -1,
    DECODER_FIRST = 0,
    DECODER_EMPTY = DECODER_FIRST,
    DECODER_FFMPEG,
    DECODER_NDL,
    DECODER_LGNC,
    DECODER_SMP,
    DECODER_DILE,
    DECODER_PI,
    DECODER_MMAL,
    DECODER_COUNT,
    DECODER_NONE = -10,
};

enum AUDIO_T {
    AUDIO_DECODER = -2,
    AUDIO_AUTO = -1,
    AUDIO_FIRST = 0,
    AUDIO_EMPTY = AUDIO_FIRST,
    AUDIO_SDL,
    AUDIO_PULSE,
    AUDIO_ALSA,
    AUDIO_NDL,
    AUDIO_COUNT,
    AUDIO_NONE = -10,
};
typedef enum AUDIO_T AUDIO;

typedef struct MODULE_DEFINITION {
    const char *name;
    const char *id;
#if FEATURE_CHECK_MODULE_OS_VERSION
    MODULE_OS_REQUIREMENT os_req;
#endif
} MODULE_DEFINITION;

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
} *PDECODER_INFO, DECODER_INFO;

typedef struct AUDIO_INFO {
    bool valid;
    int configuration;
} *PAUDIO_INFO, AUDIO_INFO;

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

