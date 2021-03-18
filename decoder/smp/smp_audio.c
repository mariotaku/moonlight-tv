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

#include "audio.h"
#include "smp_common.h"

#include <string.h>
#include <stdio.h>

static short pcmBuffer[FRAME_SIZE * MAX_CHANNEL_COUNT];
static int channelCount;

static int _init(int audioConfiguration, POPUS_MULTISTREAM_CONFIGURATION opusConfig, void *context, int arFlags)
{
  int rc;
  channelCount = opusConfig->channelCount;

  memcpy(&audioConfig, &(DirectMediaAudioConfig){DirectMediaAudioCodecOpus, channelCount, 16, opusConfig->sampleRate}, sizeof(DirectMediaAudioConfig));
  smp_player_create();
  return 0;
}

static void _cleanup()
{
  smp_player_destroy();
}
static void _start()
{
  smp_player_open();
}

static void _stop()
{
  smp_player_close();
}

static void _decode_and_play_sample(char *data, int length)
{
  if (!playerctx)
    return;
  StarfishDirectMediaPlayer_Feed(playerctx, data, length, pts, DirectMediaFeedAudio);
}

AUDIO_RENDERER_CALLBACKS audio_callbacks_smp = {
    .init = _init,
    .start = _start,
    .stop = _stop,
    .cleanup = _cleanup,
    .decodeAndPlaySample = _decode_and_play_sample,
    .capabilities = CAPABILITY_DIRECT_SUBMIT,
};
