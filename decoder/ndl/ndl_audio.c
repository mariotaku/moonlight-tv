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
#include "util/logging.h"
#include "ndl_common.h"

#include <NDL_directmedia.h>

#include <stdio.h>
#include <opus_multistream.h>

#define audio_callbacks PLUGIN_SYMBOL_NAME(audio_callbacks)

#define MAX_CHANNEL_COUNT 8
#define FRAME_SIZE 240

static OpusMSDecoder *decoder;
static short pcmBuffer[FRAME_SIZE * MAX_CHANNEL_COUNT];
static int channelCount;

static int ndl_renderer_init(int audioConfiguration, POPUS_MULTISTREAM_CONFIGURATION opusConfig, void *context, int arFlags)
{
  int rc;
  decoder = opus_multistream_decoder_create(opusConfig->sampleRate, opusConfig->channelCount, opusConfig->streams, opusConfig->coupledStreams, opusConfig->mapping, &rc);

  channelCount = opusConfig->channelCount;

  NDL_DIRECTAUDIO_DATA_INFO info = {
      .numChannel = channelCount,
      .bitPerSample = 16,
      .nodelay = 1,
      .upperThreshold = 48,
      .lowerThreshold = 16,
      .channel = NDL_DIRECTAUDIO_CH_MAIN,
      .srcType = NDL_DIRECTAUDIO_SRC_TYPE_PCM,
      .samplingFreq = NDL_DIRECTAUDIO_SAMPLING_FREQ_OF(opusConfig->sampleRate),
  };
  if (NDL_DirectAudioOpen(&info) != 0)
  {
    applog_e("NDL", "Failed to open audio: %s", NDL_DirectMediaGetError());
    return ERROR_AUDIO_OPEN_FAILED;
  }

  return 0;
}

static void ndl_renderer_cleanup()
{
  if (decoder != NULL)
    opus_multistream_decoder_destroy(decoder);

  NDL_DirectAudioClose();
}

static void ndl_renderer_decode_and_play_sample(char *data, int length)
{
  int decodeLen = opus_multistream_decode(decoder, data, length, pcmBuffer, FRAME_SIZE, 0);
  if (decodeLen > 0)
  {
    NDL_DirectAudioPlay(pcmBuffer, decodeLen * channelCount * sizeof(short));
  }
  else
  {
    applog_e("NDL", "Opus error from decode: %d", decodeLen);
  }
}

AUDIO_RENDERER_CALLBACKS audio_callbacks = {
    .init = ndl_renderer_init,
    .cleanup = ndl_renderer_cleanup,
    .decodeAndPlaySample = ndl_renderer_decode_and_play_sample,
    .capabilities = CAPABILITY_DIRECT_SUBMIT,
};
