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
    AUTO = -1,
    NONE = 0,
    FFMPEG,
    NDL,
    LGNC,
    SMP,
    DILE,
    PI,
    PLATFORM_COUNT,
};
typedef enum PLATFORM_T PLATFORM;

static const PLATFORM platform_orders[] = {
#if TARGET_WEBOS
    SMP, DILE, NDL, LGNC
#elif TARGET_LGNC
    LGNC
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

typedef struct PLATFORM_DYNLIB_DEFINITION
{
    const char *suffix;
    const char *library;
} PLATFORM_DYNLIB_DEFINITION;

typedef struct PLATFORM_DEFINITION
{
    const char *name;
    const char *id;
    const PLATFORM_DYNLIB_DEFINITION *dynlibs;
    const size_t liblen;
    const PLATFORM_SYMBOLS *symbols;
} PLATFORM_DEFINITION;

extern PLATFORM platform_pref_requested;
extern PLATFORM platform_current;
extern int platform_current_libidx;
extern PLATFORM_INFO platform_info;
extern PLATFORM_DEFINITION platform_definitions[PLATFORM_COUNT];

extern DECODER_RENDERER_CALLBACKS decoder_callbacks_dummy;

PLATFORM platform_by_id(const char *id);

PLATFORM platform_init(const char *name, int argc, char *argv[]);
PDECODER_RENDERER_CALLBACKS platform_get_video();
PAUDIO_RENDERER_CALLBACKS platform_get_audio(char *audio_device);
PVIDEO_PRESENTER_CALLBACKS platform_get_presenter();
PVIDEO_RENDER_CALLBACKS platform_get_render();
void platform_finalize();
