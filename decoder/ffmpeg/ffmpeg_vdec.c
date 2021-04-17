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

#include "common.h"
#include "ffmpeg.h"

#include <unistd.h>
#include <stdbool.h>

#include "util/memlog.h"
#include "stream/api.h"

#define DECODER_BUFFER_SIZE 2048 * 1024

static unsigned char *ffmpeg_buffer;

pthread_mutex_t mutex_ffsw = PTHREAD_MUTEX_INITIALIZER;

static int setup(int videoFormat, int width, int height, int redrawRate, void *context, int drFlags)
{
  int avc_flags = SLICE_THREADING;

  if (ffmpeg_init(videoFormat, width, height, avc_flags, SDL_BUFFER_FRAMES, sysconf(_SC_NPROCESSORS_ONLN)) < 0)
  {
    fprintf(stderr, "Couldn't initialize video decoding\n");
    return -1;
  }

  ffmpeg_buffer = malloc(DECODER_BUFFER_SIZE + AV_INPUT_BUFFER_PADDING_SIZE);
  if (ffmpeg_buffer == NULL)
  {
    fprintf(stderr, "Not enough memory\n");
    ffmpeg_destroy();
    return -1;
  }

  return 0;
}

static void cleanup()
{
  ffmpeg_destroy();
  if (ffmpeg_buffer)
  {
    free(ffmpeg_buffer);
  }
}

static int decode_submit(PDECODE_UNIT decodeUnit)
{
  if (decodeUnit->fullLength < DECODER_BUFFER_SIZE)
  {
    PLENTRY entry = decodeUnit->bufferList;
    int length = 0;
    for (PLENTRY entry = decodeUnit->bufferList; entry != NULL; entry = entry->next)
    {
      memcpy(ffmpeg_buffer + length, entry->data, entry->length);
      length += entry->length;
    }
    ffmpeg_decode(ffmpeg_buffer, length);

    if (render_queue_submit_ffmpeg && pthread_mutex_lock(&mutex_ffsw) == 0)
    {
      AVFrame *frame = ffmpeg_get_frame(false);
      if (frame != NULL)
      {
        render_next_frame_ffmpeg++;
        render_queue_submit_ffmpeg(frame);
      }

      pthread_mutex_unlock(&mutex_ffsw);
    }

    else
      fprintf(stderr, "Couldn't lock mutex\n");
  }
  else
  {
    fprintf(stderr, "Video decode buffer too small");
    exit(1);
  }

  return DR_OK;
}

DECODER_RENDERER_CALLBACKS decoder_callbacks_ffmpeg = {
    .setup = setup,
    .cleanup = cleanup,
    .submitDecodeUnit = decode_submit,
    .capabilities = CAPABILITY_SLICES_PER_FRAME(4),
};
