#include <opus_multistream.h>
#include <stdlib.h>
#include "stream/connection/session_connection.h"
#include "ss4s.h"
#include "stream/session_priv.h"

#define SAMPLES_PER_FRAME  240

static session_t *session = NULL;
static SS4S_Player *player = NULL;
static OpusMSDecoder *decoder = NULL;
static unsigned char *buffer = NULL;
static int frame_size = 0, unit_size = 0;

struct __attribute__((packed)) OpusHeader {
    __attribute__((unused)) char magic[8];
    __attribute__((unused)) uint8_t version;
    __attribute__((unused)) uint8_t channelCount;
    __attribute__((unused)) uint16_t preSkip;
    __attribute__((unused)) uint32_t inputSampleRate;
    __attribute__((unused)) int16_t outputGain;
    __attribute__((unused)) uint8_t mappingFamily;
    __attribute__((unused)) struct {
        uint8_t streamCount;
        uint8_t coupledCount;
        uint8_t mapping[255];
    } mappingTable;
};

static size_t serialize_opus_header(const OPUS_MULTISTREAM_CONFIGURATION *opusConfig, unsigned char *out);

static int aud_init(int audioConfiguration, const POPUS_MULTISTREAM_CONFIGURATION opusConfig, void *context,
                    int arFlags) {
    (void) audioConfiguration;
    (void) arFlags;
    session = context;
    player = session->player;
    SS4S_AudioCodec codec = SS4S_AUDIO_PCM_S16LE;
    size_t codecDataLen = 0;
    if (session->audio_cap.codecs & SS4S_AUDIO_OPUS) {
        codec = SS4S_AUDIO_OPUS;
        decoder = NULL;
        buffer = malloc(sizeof(struct OpusHeader));
        codecDataLen = serialize_opus_header(opusConfig, buffer);
    } else {
        int rc;
        decoder = opus_multistream_decoder_create(opusConfig->sampleRate, opusConfig->channelCount, opusConfig->streams,
                                                  opusConfig->coupledStreams, opusConfig->mapping, &rc);
        if (rc != 0) {
            return rc;
        }
        frame_size = SAMPLES_PER_FRAME * 64;
        unit_size = (int) (opusConfig->channelCount * sizeof(int16_t));
        buffer = calloc(unit_size, frame_size);
    }
    SS4S_AudioInfo info = {
            .numOfChannels = opusConfig->channelCount,
            .codec = codec,
            .appName = "Moonlight",
            .streamName = "Streaming",
            .sampleRate = opusConfig->sampleRate,
            .samplesPerFrame = SAMPLES_PER_FRAME,
            .codecData = buffer,
            .codecDataLen = codecDataLen,
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

size_t serialize_opus_header(const OPUS_MULTISTREAM_CONFIGURATION *opusConfig, unsigned char *out) {
    struct OpusHeader *header = (struct OpusHeader *) out;
    memset(header, 0, sizeof(struct OpusHeader));
    memcpy(header->magic, "OpusHead", 8);
    header->version = 1;
    header->channelCount = opusConfig->channelCount;
    header->preSkip = SDL_SwapLE16(0);
    header->inputSampleRate = SDL_SwapLE32(opusConfig->sampleRate);
    header->outputGain = SDL_SwapLE16(0);
    if (opusConfig->streams > 2) {
        header->mappingFamily = 1;
        header->mappingTable.streamCount = opusConfig->streams;
        header->mappingTable.coupledCount = opusConfig->coupledStreams;
        for (int i = 0; i < opusConfig->streams; i++) {
            header->mappingTable.mapping[i] = opusConfig->mapping[i];
        }
        return 21 + opusConfig->streams;
    }
    return 19;
}

AUDIO_RENDERER_CALLBACKS ss4s_aud_callbacks = {
        .init = aud_init,
        .cleanup = aud_cleanup,
        .decodeAndPlaySample = aud_feed,
        .capabilities = CAPABILITY_DIRECT_SUBMIT,
};