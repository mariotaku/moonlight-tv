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
#include "ringbuf.h"
#include "util/logging.h"

#include <SDL.h>

#include <Limelight.h>
#include <opus_multistream.h>

#define MAX_CHANNEL_COUNT 6

static SDL_AudioDeviceID dev = 0;
static OpusMSDecoder *decoder = NULL;
static sdlaud_ringbuf *ringbuf = NULL;
static unsigned char *pcmBuffer = NULL;
static unsigned char *readbuf = NULL;
static size_t readbuf_size = 0;
static int channelCount, samplesPerFrame, sampleRate;

static void sdlaud_callback(void *userdata, Uint8 *stream, int len);

static int setup(int audioConfiguration, POPUS_MULTISTREAM_CONFIGURATION opusConfig, void *context, int arFlags) {
    int rc;
    samplesPerFrame = opusConfig->samplesPerFrame;
    channelCount = opusConfig->channelCount;
    sampleRate = opusConfig->sampleRate;

    decoder = opus_multistream_decoder_create(sampleRate, channelCount, opusConfig->streams, opusConfig->coupledStreams,
                                              opusConfig->mapping, &rc);

    ringbuf = sdlaud_ringbuf_new(opusConfig->samplesPerFrame * MAX_CHANNEL_COUNT * 8);
    pcmBuffer = malloc(sizeof(short) * channelCount * samplesPerFrame);

    SDL_InitSubSystem(SDL_INIT_AUDIO);

    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.callback = sdlaud_callback;
    want.freq = sampleRate;
    want.format = AUDIO_S16LSB;
    want.channels = channelCount;
    want.samples = opusConfig->samplesPerFrame * 2;

    dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (dev == 0) {
        applog_e("SDLAud", "Failed to open audio: %s", SDL_GetError());
        return -1;
    } else {
        if (have.format != want.format) // we let this one thing change.
            applog_w("SDLAud", "We didn't get requested audio format.");
        SDL_PauseAudioDevice(dev, 0); // start audio playing.
    }

    return 0;
}

static void cleanup() {
    if (decoder != NULL) {
        opus_multistream_decoder_destroy(decoder);
        decoder = NULL;
    }
    if (ringbuf != NULL) {
        sdlaud_ringbuf_delete(ringbuf);
        ringbuf = NULL;
    }
    if (readbuf != NULL) {
        SDL_free(readbuf);
        readbuf_size = 0;
        readbuf = NULL;
    }
    if (pcmBuffer != NULL) {
        SDL_free(pcmBuffer);
        pcmBuffer = NULL;
    }
    SDL_CloseAudioDevice(dev);
    dev = 0;
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

static void queue(char *data, int length) {
    int decodeLen = opus_multistream_decode(decoder, (unsigned char *) data, length, pcmBuffer, samplesPerFrame, 0);
    if (decodeLen > 0) {
        size_t write_size = sdlaud_ringbuf_write(ringbuf, (unsigned char *) pcmBuffer,
                                                 decodeLen * channelCount * sizeof(short));
        if (!write_size) {
            applog_w("SDLAud", "ring buffer overflow, clean the whole buffer");
            sdlaud_ringbuf_clear(ringbuf);
        }
    } else {
        applog_e("SDLAud", "Opus error from decode: %d", decodeLen);
    }
}

static void sdlaud_callback(void *userdata, Uint8 *stream, int len) {
    (void) userdata;
    if (readbuf_size < len) {
        readbuf = SDL_realloc(readbuf, len);
    }
    size_t read_size = sdlaud_ringbuf_read(ringbuf, readbuf, len);
    if (read_size > 0) {
        SDL_memset(stream, 0, len);
        SDL_MixAudio(stream, readbuf, read_size, SDL_MIX_MAXVOLUME);
    }
}

AUDIO_RENDERER_CALLBACKS audio_callbacks_sdl = {
        .init = setup,
        .cleanup = cleanup,
        .decodeAndPlaySample = queue,
        .capabilities = CAPABILITY_DIRECT_SUBMIT | CAPABILITY_SUPPORTS_ARBITRARY_AUDIO_DURATION,
};

bool audio_check_sdl(PAUDIO_INFO ainfo) {
    ainfo->valid = true;
    ainfo->configuration = AUDIO_CONFIGURATION_STEREO;
    return true;
}
