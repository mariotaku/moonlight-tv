/*
 * This file is part of Moonlight Embedded.
 *
 * Copyright (C) 2015 Iwan Timmer
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

#include "video.h"

#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <NDL_directmedia.h>

// 2MB decode size should be fairly enough for everything
#define DECODER_BUFFER_SIZE 2048 * 1024

static char *ndl_buffer;

static int ndl_setup(int videoFormat, int width, int height, int redrawRate, void *context, int drFlags)
{
  NDL_DIRECTVIDEO_DATA_INFO info = {width, height};
  if (NDL_DirectVideoOpen(&info) < 0)
  {
    fprintf(stderr, "Couldn't initialize video decoding\n");
    return -1;
  }
  ndl_buffer = malloc(DECODER_BUFFER_SIZE);
  if (ndl_buffer == NULL)
  {
    fprintf(stderr, "Not enough memory\n");
    NDL_DirectVideoClose();
    return -1;
  }

  return 0;
}

static void ndl_cleanup()
{
  NDL_DirectVideoClose();
}

static int ndl_submit_decode_unit(PDECODE_UNIT decodeUnit)
{
  if (decodeUnit->fullLength < DECODER_BUFFER_SIZE)
  {
    PLENTRY entry = decodeUnit->bufferList;
    int length = 0;
    while (entry != NULL)
    {
      memcpy(ndl_buffer + length, entry->data, entry->length);
      length += entry->length;
      entry = entry->next;
    }
    NDL_DirectVideoPlay(ndl_buffer, length);
  }
  else
  {
    fprintf(stderr, "Video decode buffer too small, skip this frame");
    return DR_NEED_IDR;
  }

  return DR_OK;
}

DECODER_RENDERER_CALLBACKS decoder_callbacks_ndl = {
    .setup = ndl_setup,
    .cleanup = ndl_cleanup,
    .submitDecodeUnit = ndl_submit_decode_unit,
    .capabilities = CAPABILITY_SLICES_PER_FRAME(4) | CAPABILITY_DIRECT_SUBMIT,
};
