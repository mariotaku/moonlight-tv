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

#include "ndl_common.h"
#include "stream/module/api.h"
#include "util/logging.h"

#include <NDL_directmedia.h>

#include <stdio.h>
#include <string.h>

#define audio_callbacks PLUGIN_SYMBOL_NAME(audio_callbacks)

#define MAX_CHANNEL_COUNT 8
#define FRAME_SIZE 240

static int channelCount;

static int ndl_renderer_init(int audioConfiguration, POPUS_MULTISTREAM_CONFIGURATION opusConfig, void *context, int arFlags)
{
  channelCount = opusConfig->channelCount;
  media_info.audio.type = NDL_AUDIO_TYPE_OPUS;
  media_info.audio.opus.channels = opusConfig->channelCount;
  media_info.audio.opus.sampleRate = (double)opusConfig->sampleRate / 1000.0f;
  media_info.audio.opus.streamHeader = NULL;
  if (media_reload() != 0)
  {
    applog_e("NDL", "Failed to open audio: %s", NDL_DirectMediaGetError());
    return ERROR_AUDIO_OPEN_FAILED;
  }
  return 0;
}

static void ndl_renderer_cleanup()
{
  media_unload();
  memset(&media_info.audio, 0, sizeof(media_info.audio));
}

static void ndl_renderer_decode_and_play_sample(char *data, int length)
{
  if (!media_loaded)
    return;
  if (NDL_DirectAudioPlay(data, length, 0) != 0)
  {
    applog_e("NDL", "Error playing sample");
  }
}

AUDIO_RENDERER_CALLBACKS audio_callbacks = {
    .init = ndl_renderer_init,
    .cleanup = ndl_renderer_cleanup,
    .decodeAndPlaySample = ndl_renderer_decode_and_play_sample,
    .capabilities = CAPABILITY_DIRECT_SUBMIT,
};
