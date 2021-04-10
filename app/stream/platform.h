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

#define IS_EMBEDDED(SYSTEM) SYSTEM != SDL

#define PLATFORM_HDR_NONE 0
#define PLATFORM_HDR_SUPPORTED 1
#define PLATFORM_HDR_ALWAYS 2

#ifdef DECODER_PLATFORM_NAME
// Coming from https://stackoverflow.com/a/1489985/859190
#define DECODER_DECL_PASTER(x, y) x##_##y
#define DECODER_DECL_EVALUATOR(x, y) DECODER_DECL_PASTER(x, y)
#define DECODER_SYMBOL_NAME(name) DECODER_DECL_EVALUATOR(name, DECODER_PLATFORM_NAME)
#endif

typedef struct PLATFORM_INFO
{
    bool valid;
    unsigned int vrank;
    unsigned int arank;
    bool hevc;
    int hdr;
    int colorSpace;
    int colorRange;
    int maxBitrate;
} * PPLATFORM_INFO, PLATFORM_INFO;

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

PLATFORM platform_current;
PLATFORM_INFO platform_states[FAKE + 1];

PLATFORM platform_init(const char *name, int argc, char *argv[]);
PDECODER_RENDERER_CALLBACKS platform_get_video(PLATFORM system);
PAUDIO_RENDERER_CALLBACKS platform_get_audio(PLATFORM system, char *audio_device);
PVIDEO_PRESENTER_CALLBACKS platform_get_presenter(PLATFORM system);

const char *platform_name(enum PLATFORM_T system);

void platform_finalize(enum PLATFORM_T system);
