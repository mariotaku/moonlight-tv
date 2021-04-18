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

#include "stream/api.h"
#include "ndl_common.h"

#include <NDL_directmedia.h>

#include <stdio.h>

#define audio_callbacks PLUGIN_SYMBOL_NAME(audio_callbacks)

#define MAX_CHANNEL_COUNT 8
#define FRAME_SIZE 240

static int channelCount;

static int ndl_renderer_init(int audioConfiguration, POPUS_MULTISTREAM_CONFIGURATION opusConfig, void *context, int arFlags)
{
  channelCount = opusConfig->channelCount;
  media_info.audio.type = NDL_AUDIO_TYPE_OPUS;
  media_info.audio.opus.channels = opusConfig->channelCount;
  media_info.audio.opus.sampleRate = (double) opusConfig->sampleRate / 1000.0f;
  media_info.audio.opus.streamHeader = NULL;
  // Unload player before reloading
  if (media_loaded && NDL_DirectMediaUnload() != 0)
    return -1;
  if (NDL_DirectMediaLoad(&media_info, media_load_callback))
  {
    printf("Failed to open audio: %s\n", NDL_DirectMediaGetError());
    return -1;
  }
  media_loaded = true;
  return 0;
}

static void ndl_renderer_cleanup()
{
  if (media_loaded)
  {
    NDL_DirectMediaUnload();
    media_loaded = false;
  }
}

static void ndl_renderer_decode_and_play_sample(char *data, int length)
{
  if (NDL_DirectAudioPlay(data, length, 0) != 0)
  {
    printf("[NDL] Error playing sample\n");
  }
}

AUDIO_RENDERER_CALLBACKS audio_callbacks = {
    .init = ndl_renderer_init,
    .cleanup = ndl_renderer_cleanup,
    .decodeAndPlaySample = ndl_renderer_decode_and_play_sample,
    .capabilities = CAPABILITY_DIRECT_SUBMIT,
};
