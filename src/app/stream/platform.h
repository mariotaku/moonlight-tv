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

bool decoder_max_dimension(int *width, int *height);

int decoder_max_framerate();

const char *module_geterror();

