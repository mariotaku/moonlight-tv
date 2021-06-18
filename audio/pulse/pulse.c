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

#include "stream/module/api.h"
#include "util/logging.h"

#include <stdio.h>
#include <stdlib.h>

#include <Limelight.h>
#include <opus_multistream.h>
#include <pulse/simple.h>
#include <pulse/channelmap.h>
#include <pulse/error.h>

#define MAX_CHANNEL_COUNT 8
#define FRAME_SIZE 240

static OpusMSDecoder *decoder;
static pa_simple *dev = NULL;
static short pcmBuffer[FRAME_SIZE * MAX_CHANNEL_COUNT];
static int channelCount;

static int pulse_renderer_init(int audioConfiguration, POPUS_MULTISTREAM_CONFIGURATION opusConfig, void *context, int arFlags)
{
    int rc, error;

    channelCount = opusConfig->channelCount;

    decoder = opus_multistream_decoder_create(opusConfig->sampleRate, opusConfig->channelCount,
                                              opusConfig->streams, opusConfig->coupledStreams, opusConfig->mapping, &rc);

    pa_sample_spec spec = {
        .format = PA_SAMPLE_S16LE,
        .rate = opusConfig->sampleRate,
        .channels = opusConfig->channelCount,
    };
    pa_buffer_attr buffer_attr = {
        .maxlength = -1,
        .tlength = sizeof(short) * channelCount,
        .prebuf = -1,
        .minreq = -1,
        .fragsize = -1,
    };
    pa_channel_map channel_map;
    pa_channel_map *pchannel_map = pa_channel_map_init(&channel_map);
    pchannel_map->channels = channelCount;
    switch (channelCount)
    {
    case 8:
        pchannel_map->map[6] = PA_CHANNEL_POSITION_SIDE_LEFT;
        pchannel_map->map[7] = PA_CHANNEL_POSITION_SIDE_RIGHT;
    // Intentionally fallthrough
    case 6:
        pchannel_map->map[2] = PA_CHANNEL_POSITION_FRONT_CENTER;
        pchannel_map->map[3] = PA_CHANNEL_POSITION_LFE;
        pchannel_map->map[4] = PA_CHANNEL_POSITION_REAR_LEFT;
        pchannel_map->map[5] = PA_CHANNEL_POSITION_REAR_RIGHT;
    // Intentionally fallthrough
    default:
        pchannel_map->map[0] = PA_CHANNEL_POSITION_FRONT_LEFT;
        pchannel_map->map[1] = PA_CHANNEL_POSITION_FRONT_RIGHT;
    }

    char *audio_device = (char *)context;
    dev = pa_simple_new(audio_device, "Moonlight TV", PA_STREAM_PLAYBACK, NULL, "Streaming", &spec, pchannel_map, &buffer_attr, &error);

    if (!dev)
    {
        applog_e("PulseAudio", "Failed to create device: %s\n", pa_strerror(error));
        return -1;
    }
    applog_d("PulseAudio", "Audio device created.");

    return 0;
}

static void pulse_renderer_decode_and_play_sample(char *data, int length)
{
    int decodeLen = opus_multistream_decode(decoder, data, length, pcmBuffer, FRAME_SIZE, 0);
    if (decodeLen > 0)
    {
        int error;
        int rc = pa_simple_write(dev, pcmBuffer, decodeLen * sizeof(short) * channelCount, &error);

        if (rc < 0)
            applog_w("PulseAudio", "playback error: %s", pa_strerror(error));
    }
    else
    {
        applog_w("PulseAudio", "Opus error from decode: %d", decodeLen);
    }
}

static void pulse_renderer_cleanup()
{
    pa_simple_free(dev);
}

AUDIO_RENDERER_CALLBACKS audio_callbacks_pulse = {
    .init = pulse_renderer_init,
    .cleanup = pulse_renderer_cleanup,
    .decodeAndPlaySample = pulse_renderer_decode_and_play_sample,
    .capabilities = CAPABILITY_DIRECT_SUBMIT,
};
