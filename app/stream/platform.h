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

#include <dlfcn.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "api.h"

enum PLATFORM_T
{
    NONE = 0,
    FFMPEG,
    NDL,
    LGNC,
    SMP,
    SMP_ACB,
    DILE,
    DILE_LEGACY,
    PI,
    FAKE,
    PLATFORM_COUNT,
};
typedef enum PLATFORM_T PLATFORM;

static const PLATFORM platform_orders[] = {
#if TARGET_WEBOS
    SMP, SMP_ACB, DILE_LEGACY, NDL, LGNC, FFMPEG
#elif TARGET_LGNC
    LGNC, FFMPEG
#elif TARGET_RASPI
    PI, FFMPEG
#else
    FFMPEG
#endif
};
static const size_t platform_orders_len = sizeof(platform_orders) / sizeof(PLATFORM);

typedef bool (*PLATFORM_INIT_FN)(int argc, char *argv[]);
typedef bool (*PLATFORM_CHECK_FN)(PPLATFORM_INFO);
typedef void (*PLATFORM_FINALIZE_FN)();

typedef struct PLATFORM_SYMBOLS_T
{
    bool valid;
    PLATFORM_INIT_FN init;
    PLATFORM_CHECK_FN check;
    PLATFORM_FINALIZE_FN finalize;
    PAUDIO_RENDERER_CALLBACKS adec;
    PDECODER_RENDERER_CALLBACKS vdec;
    PVIDEO_PRESENTER_CALLBACKS pres;
    PVIDEO_RENDER_CALLBACKS rend;
} PLATFORM_SYMBOLS, PPLATFORM_SYMBOLS;

typedef struct PLATFORM_DEFINITION
{
    const char *name;
    const char *id;
    const char *library;
    const PLATFORM_SYMBOLS *symbols;
} PLATFORM_DEFINITION;

PLATFORM platform_default;
PLATFORM_INFO platforms_info[PLATFORM_COUNT];
PLATFORM_DEFINITION platform_definitions[PLATFORM_COUNT];
int platform_available_count;

PLATFORM_SYMBOLS platform_lgnc;
DECODER_RENDERER_CALLBACKS decoder_callbacks_dummy;

PLATFORM platforms_init(const char *name, int argc, char *argv[]);
PDECODER_RENDERER_CALLBACKS platform_get_video(PLATFORM platform);
PAUDIO_RENDERER_CALLBACKS platform_get_audio(PLATFORM platform, char *audio_device, PLATFORM vplatform);
PVIDEO_PRESENTER_CALLBACKS platform_get_presenter(PLATFORM platform);
PVIDEO_RENDER_CALLBACKS platform_get_render(PLATFORM platform);

PLATFORM platform_preferred_audio(PLATFORM vplatform);
PLATFORM platform_by_id(const char *id);

bool platform_render_video();

void platform_finalize(enum PLATFORM_T platform);
