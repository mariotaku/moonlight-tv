#include <stdlib.h>
#include <assert.h>

#include <opus_multistream.h>

#include "ss4s.h"
#include "stream/connection/session_connection.h"
#include "stream/session_priv.h"
#include "logging.h"

#define SAMPLES_PER_FRAME  240

static session_t *session = NULL;
static SS4S_Player *player = NULL;
static OpusMSDecoder *decoder = NULL;
static unsigned char *buffer = NULL;
static int frame_size = 0, unit_size = 0;

AUDIO_INFO audio_stream_info;

static size_t opus_head_serialize(const OPUS_MULTISTREAM_CONFIGURATION *config, unsigned char *data);

static int aud_init(int audioConfiguration, const POPUS_MULTISTREAM_CONFIGURATION opusConfig, void *context,
                    int arFlags) {
    (void) audioConfiguration;
    (void) arFlags;
    memset(&audio_stream_info, 0, sizeof(audio_stream_info));
    session = context;
    player = session->player;
    SS4S_AudioCodec codec = SS4S_AUDIO_PCM_S16LE;
    size_t codecDataLen = 0;
    SS4S_AudioInfo info = {
            .numOfChannels = opusConfig->channelCount,
            .appName = "Moonlight",
            .streamName = "Streaming",
            .sampleRate = opusConfig->sampleRate,
            .samplesPerFrame = SAMPLES_PER_FRAME,
    };
    if (session->audio_cap.codecs & SS4S_AUDIO_OPUS && SS4S_GetAudioPreferredCodecs(&info) & SS4S_AUDIO_OPUS) {
        codec = SS4S_AUDIO_OPUS;
        decoder = NULL;
        buffer = calloc(1024, sizeof(unsigned char));
        assert(buffer != NULL);
        codecDataLen = opus_head_serialize(opusConfig, buffer);
    } else {
        int rc;
        decoder = opus_multistream_decoder_create(opusConfig->sampleRate, opusConfig->channelCount, opusConfig->streams,
                                                  opusConfig->coupledStreams, opusConfig->mapping, &rc);
        if (rc != 0) {
            return rc;
        }
        frame_size = opusConfig->samplesPerFrame;
        unit_size = (int) (opusConfig->channelCount * sizeof(int16_t));
        buffer = calloc(unit_size, frame_size);
    }
    audio_stream_info.format = SS4S_AudioCodecName(codec);
    info.codec = codec;
    info.codecData = buffer;
    info.codecDataLen = codecDataLen;
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
    if (buffer != NULL) {
        free(buffer);
        buffer = NULL;
    }
    session = NULL;
}

static void aud_feed(char *sampleData, int sampleLength) {
    if (decoder != NULL) {
        int decode_len = opus_multistream_decode(decoder, (unsigned char *) sampleData, sampleLength,
                                                 (opus_int16 *) buffer, frame_size, 0);
        SS4S_PlayerAudioFeed(player, buffer, unit_size * decode_len);
    } else {
        SS4S_PlayerAudioFeed(player, (unsigned char *) sampleData, sampleLength);
    }
}

static size_t opus_head_serialize(const OPUS_MULTISTREAM_CONFIGURATION *config, unsigned char *data) {
    unsigned char *ptr = data;
    // 1. Magic Signature:
    ptr = memcpy(ptr, "OpusHead", 8) + 8;
    // 2. Version (8 bits, unsigned):
    *ptr++ = 1;
    // 3. Output Channel Count 'C' (8 bits, unsigned):
    *ptr++ = config->channelCount;
    // 4. Pre-skip (16 bits, unsigned, little-endian):
    uint16_t preskip = SDL_SwapLE16(0);
    ptr = memcpy(ptr, &preskip, 2) + 2;
    // 5. Input Sample Rate (32 bits, unsigned, little-endian):
    uint32_t sampleRate = SDL_SwapLE32(config->sampleRate);
    ptr = memcpy(ptr, &sampleRate, 4) + 4;
    // 6. Output Gain (16 bits, signed, little-endian):
    int16_t outputGain = SDL_SwapLE16(0);
    ptr = memcpy(ptr, &outputGain, 2) + 2;
    // 7. Channel Mapping Family (8 bits, unsigned):
    *ptr++ = 1;
    // 8.1. Stream Count 'N' (8 bits, unsigned):
    *ptr++ = config->streams;
    // 8.2. Coupled Stream Count 'M' (8 bits, unsigned):
    *ptr++ = config->coupledStreams;
    // 8.3. Channel Mapping (8*C bits):
    ptr = memcpy(ptr, config->mapping, config->channelCount) + config->channelCount;
    return ptr - data;
}

AUDIO_RENDERER_CALLBACKS ss4s_aud_callbacks = {
        .init = aud_init,
        .cleanup = aud_cleanup,
        .decodeAndPlaySample = aud_feed,
        .capabilities = CAPABILITY_DIRECT_SUBMIT,
};