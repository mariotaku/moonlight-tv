#include <opus_multistream.h>
#include <stdlib.h>
#include "stream/connection/session_connection.h"
#include "ss4s.h"
#include "stream/session_priv.h"

static session_t *session = NULL;
static SS4S_Player *player = NULL;
static OpusMSDecoder *decoder = NULL;
static unsigned char *pcmbuf = NULL;
static int frame_size = 0, unit_size = 0;

static int aud_init(int audioConfiguration, const POPUS_MULTISTREAM_CONFIGURATION opusConfig, void *context,
                    int arFlags) {
    session = context;
    player = session->player;
    int rc;
    decoder = opus_multistream_decoder_create(opusConfig->sampleRate, opusConfig->channelCount, opusConfig->streams,
                                              opusConfig->coupledStreams, opusConfig->mapping, &rc);
    if (rc != 0) {
        return rc;
    }
    SS4S_AudioInfo info = {
            .numOfChannels = opusConfig->channelCount,
            .codec = SS4S_AUDIO_PCM_S16LE,
            .appName = "Moonlight",
            .streamName = "Streaming",
            .sampleRate = opusConfig->sampleRate,
            .samplesPerFrame = 240,
    };
    frame_size = info.samplesPerFrame * 64;
    unit_size = (int) (info.numOfChannels * sizeof(int16_t));
    pcmbuf = calloc(unit_size, frame_size);
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
    int decode_len = opus_multistream_decode(decoder, (unsigned char *) sampleData, sampleLength, (opus_int16 *) pcmbuf,
                                             frame_size, 0);
    SS4S_PlayerAudioFeed(player, pcmbuf, unit_size * decode_len);
}

AUDIO_RENDERER_CALLBACKS ss4s_aud_callbacks = {
        .init = aud_init,
        .cleanup = aud_cleanup,
        .decodeAndPlaySample = aud_feed,
        .capabilities = CAPABILITY_DIRECT_SUBMIT,
};