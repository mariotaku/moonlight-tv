#include <opus_multistream.h>
#include <stdlib.h>
#include "stream/connection/session_connection.h"
#include "ss4s.h"
#include "stream/session_priv.h"
#include "logging.h"

static session_t *session = NULL;
static SS4S_Player *player = NULL;
static OpusMSDecoder *decoder = NULL;
static unsigned char *pcmbuf = NULL;
static int frame_size = 0, unit_size = 0;

static int aud_init(int audioFormat, int audioConfiguration, const OPUS_MULTISTREAM_CONFIGURATION *opusConfig,
                    void *context, int arFlags) {
    (void) audioConfiguration;
    (void) arFlags;
    session = context;
    player = session->player;
    SS4S_AudioCodec codec = SS4S_AUDIO_PCM_S16LE;
    if (audioFormat == AUDIO_FORMAT_EAC3) {
        codec = SS4S_AUDIO_EAC3;
        decoder = NULL;
        pcmbuf = NULL;
    } else if (audioFormat == AUDIO_FORMAT_AC3) {
        codec = SS4S_AUDIO_AC3;
        decoder = NULL;
        pcmbuf = NULL;
    } else if (audioFormat & AUDIO_FORMAT_MASK_OPUS) {
        if (session->audio_cap.codecs & SS4S_AUDIO_OPUS) {
            codec = SS4S_AUDIO_OPUS;
            decoder = NULL;
            pcmbuf = NULL;
        } else {
            int rc;
            decoder = opus_multistream_decoder_create(opusConfig->sampleRate, opusConfig->channelCount,
                                                      opusConfig->streams,
                                                      opusConfig->coupledStreams, opusConfig->mapping, &rc);
            if (rc != 0) {
                return rc;
            }
            unit_size = (int) (opusConfig->channelCount * sizeof(int16_t));
            frame_size = opusConfig->samplesPerFrame;
            pcmbuf = calloc(unit_size, frame_size);
        }
    } else {
        commons_log_error("Audio", "Unsupported audio format: %04x", audioFormat);
        return SS4S_AUDIO_OPEN_UNSUPPORTED_CODEC;
    }
    SS4S_AudioInfo info = {
            .numOfChannels = opusConfig->channelCount,
            .codec = codec,
            .appName = "Moonlight",
            .streamName = "Streaming",
            .sampleRate = opusConfig->sampleRate,
            .samplesPerFrame = opusConfig->samplesPerFrame,
    };
    return SS4S_PlayerAudioOpen(player, &info);
}

static void aud_cleanup() {
    if (player != NULL) {
        SS4S_PlayerAudioClose(player);
        player = NULL;
    }
    if (decoder != NULL) {
        opus_multistream_decoder_destroy(decoder);
        decoder = NULL;
    }
    if (pcmbuf != NULL) {
        free(pcmbuf);
        pcmbuf = NULL;
    }
    session = NULL;
}

static void aud_feed(char *sampleData, int sampleLength) {
    if (decoder != NULL) {
        int decode_len = opus_multistream_decode(decoder, (unsigned char *) sampleData, sampleLength,
                                                 (opus_int16 *) pcmbuf, frame_size, 0);
        SS4S_PlayerAudioFeed(player, pcmbuf, unit_size * decode_len);
    } else {
        SS4S_PlayerAudioFeed(player, (unsigned char *) sampleData, sampleLength);
    }
}

AUDIO_RENDERER_CALLBACKS ss4s_aud_callbacks = {
        .init2 = aud_init,
        .cleanup = aud_cleanup,
        .decodeAndPlaySample = aud_feed,
        .capabilities = CAPABILITY_DIRECT_SUBMIT,
};