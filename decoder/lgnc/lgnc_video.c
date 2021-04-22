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

#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Limelight.h>
#include <lgnc_directvideo.h>

#include "stream/module/api.h"
#include "util/logging.h"

// 2MB decode size should be fairly enough for everything
#define DECODER_BUFFER_SIZE 2048 * 1024

static char *lgnc_buffer;

static int lgnc_setup(int videoFormat, int width, int height, int redrawRate, void *context, int drFlags)
{
  LGNC_VDEC_DATA_INFO_T info = {
      .width = width,
      .height = height,
      .vdecFmt = LGNC_VDEC_FMT_H264,
      .trid_type = LGNC_VDEC_3D_TYPE_NONE};
  if (LGNC_DIRECTVIDEO_Open(&info) != 0)
  {
    applog_e("LGNC", "Couldn't initialize video decoding");
    return ERROR_DECODER_OPEN_FAILED;
  }
  lgnc_buffer = malloc(DECODER_BUFFER_SIZE);
  if (lgnc_buffer == NULL)
  {
    applog_e("LGNC", "Not enough memory");
    LGNC_DIRECTVIDEO_Close();
    return ERROR_OUT_OF_MEMORY;
  }

  return 0;
}

static void lgnc_cleanup()
{
  LGNC_DIRECTVIDEO_Close();
  if (lgnc_buffer)
  {
    free(lgnc_buffer);
  }
}

static int lgnc_submit_decode_unit(PDECODE_UNIT decodeUnit)
{
  if (decodeUnit->fullLength < DECODER_BUFFER_SIZE)
  {
    int length = 0;
    for (PLENTRY entry = decodeUnit->bufferList; entry != NULL; entry = entry->next)
    {
      memcpy(lgnc_buffer + length, entry->data, entry->length);
      length += entry->length;
    }
    if (LGNC_DIRECTVIDEO_Play(lgnc_buffer, length) != 0)
    {
      applog_w("LGNC", "LGNC_DIRECTVIDEO_Play returned non zero");
      return DR_NEED_IDR;
    }
  }
  else
  {
    applog_w("LGNC", "Video decode buffer too small, skip this frame");
    return DR_NEED_IDR;
  }

  return DR_OK;
}

DECODER_RENDERER_CALLBACKS decoder_callbacks_lgnc = {
    .setup = lgnc_setup,
    .cleanup = lgnc_cleanup,
    .submitDecodeUnit = lgnc_submit_decode_unit,
    .capabilities = CAPABILITY_SLICES_PER_FRAME(4) | CAPABILITY_DIRECT_SUBMIT,
};
