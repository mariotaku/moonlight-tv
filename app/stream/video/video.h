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

#include <Limelight.h>

#include <stdbool.h>

#include "presenter.h"

#define DISPLAY_FULLSCREEN 1
#define ENABLE_HARDWARE_ACCELERATION_1 2
#define ENABLE_HARDWARE_ACCELERATION_2 4
#define DISPLAY_ROTATE_MASK 24
#define DISPLAY_ROTATE_90 8
#define DISPLAY_ROTATE_180 16
#define DISPLAY_ROTATE_270 24

#define INIT_EGL 1
#define INIT_VDPAU 2
#define INIT_VAAPI 3

#ifdef OS_WEBOS
extern DECODER_RENDERER_CALLBACKS decoder_callbacks_ndl;
extern DECODER_RENDERER_CALLBACKS decoder_callbacks_lgnc;
#endif

#ifdef HAVE_SDL
extern DECODER_RENDERER_CALLBACKS decoder_callbacks_sdl;
#endif

#ifdef HAVE_PI
extern DECODER_RENDERER_CALLBACKS decoder_callbacks_pi;
extern VIDEO_PRESENTER_CALLBACKS presenter_callbacks_pi;
#endif

#ifdef HAVE_MMAL
extern DECODER_RENDERER_CALLBACKS decoder_callbacks_mmal;
#endif

extern DECODER_RENDERER_CALLBACKS decoder_callbacks_dummy;