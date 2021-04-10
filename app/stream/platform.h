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

#include "video/presenter.h"
#include "api.h"

enum PLATFORM_T
{
    NONE = 0,
    SDL,
    NDL,
    LGNC,
    SMP,
    SMP_ACB,
    DILE,
    DILE_LEGACY,
    FAKE
};
typedef enum PLATFORM_T PLATFORM;

typedef bool (*PLATFORM_INIT_FN)(int argc, char *argv[]);
typedef bool (*PLATFORM_CHECK_FN)(PPLATFORM_INFO);
typedef void (*PLATFORM_FINALIZE_FN)();

typedef struct PLATFORM_SYMBOLS_T
{
    PLATFORM_INIT_FN init;
    PLATFORM_CHECK_FN check;
    PLATFORM_FINALIZE_FN finalize;
    PAUDIO_RENDERER_CALLBACKS adec;
    PDECODER_RENDERER_CALLBACKS vdec;
    PVIDEO_PRESENTER_CALLBACKS pres;
} PLATFORM_SYMBOLS, PPLATFORM_SYMBOLS;

PLATFORM platform_current;
PLATFORM_INFO platform_states[FAKE + 1];

PLATFORM_SYMBOLS platform_sdl;

PLATFORM platform_init(const char *name, int argc, char *argv[]);
PDECODER_RENDERER_CALLBACKS platform_get_video(PLATFORM platform);
PAUDIO_RENDERER_CALLBACKS platform_get_audio(PLATFORM platform, char *audio_device);
PVIDEO_PRESENTER_CALLBACKS platform_get_presenter(PLATFORM platform);

const char *platform_name(enum PLATFORM_T platform);

void platform_finalize(enum PLATFORM_T platform);
