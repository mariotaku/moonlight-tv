#include <opus_multistream.h>
#include <stdlib.h>
#include "stream/connection/session_connection.h"
#include "ss4s.h"
#include "stream/session_priv.h"
#include <fdk-aac/FDK_audio.h>
#include <fdk-aac/aacenc_lib.h>
#include <assert.h>
#include "ringbuf.h"
#include "logging.h"

#define SAMPLES_PER_FRAME  240

static session_t *session = NULL;
static SS4S_Player *player = NULL;
static OpusMSDecoder *decoder = NULL;
static HANDLE_AACENCODER aac_encoder = NULL;
static sdlaud_ringbuf *a52_ringbuf = NULL;
static unsigned char *buffer = NULL;
static unsigned char *buffer2 = NULL;
static int frame_size = 0, unit_size = 0, channel_count = 0;

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
    SS4S_AudioCodec codec;
    size_t codecDataLen = 0;
    int samplesPerFrame = SAMPLES_PER_FRAME;
    if (session->audio_cap.codecs & SS4S_AUDIO_OPUS) {
        codec = SS4S_AUDIO_OPUS;
        decoder = NULL;
        aac_encoder = NULL;
        a52_ringbuf = NULL;
        buffer = malloc(sizeof(struct OpusHeader));
        codecDataLen = serialize_opus_header(opusConfig, buffer);
    } else {
        codec = SS4S_AUDIO_AAC;
        int rc;
        decoder = opus_multistream_decoder_create(opusConfig->sampleRate, opusConfig->channelCount, opusConfig->streams,
                                                  opusConfig->coupledStreams, opusConfig->mapping, &rc);
        if (rc != 0) {
            return rc;
        }
        aacEncOpen(&aac_encoder, 0, opusConfig->channelCount);
        aacEncoder_SetParam(aac_encoder, AACENC_AOT, AOT_AAC_LC);
        aacEncoder_SetParam(aac_encoder, AACENC_BITRATE, 320000);
        aacEncoder_SetParam(aac_encoder, AACENC_SAMPLERATE, opusConfig->sampleRate);
        aacEncoder_SetParam(aac_encoder, AACENC_CHANNELMODE, opusConfig->channelCount == 6 ? MODE_1_2_2_1 : MODE_2);
        aacEncoder_SetParam(aac_encoder, AACENC_TRANSMUX, TT_MP4_ADTS);

        channel_count = opusConfig->channelCount;
        frame_size = SAMPLES_PER_FRAME * 64;
        unit_size = (int) (opusConfig->channelCount * sizeof(int16_t));
        buffer = calloc(unit_size, frame_size);
        buffer2 = calloc(unit_size, frame_size);
        a52_ringbuf = sdlaud_ringbuf_new(unit_size * 1024 * 2);
        samplesPerFrame = 1024;
    }
    SS4S_AudioInfo info = {
            .numOfChannels = opusConfig->channelCount,
            .codec = codec,
            .appName = "Moonlight",
            .streamName = "Streaming",
            .sampleRate = opusConfig->sampleRate,
            .samplesPerFrame = samplesPerFrame,
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
    if (buffer2 != NULL) {
        free(buffer2);
        buffer2 = NULL;
    }
    if (aac_encoder != NULL) {
        aacEncClose(&aac_encoder);
        aac_encoder = NULL;
    }
    if (a52_ringbuf != NULL) {
        sdlaud_ringbuf_delete(a52_ringbuf);
        a52_ringbuf = NULL;
    }
    session = NULL;
}

static void aud_feed(char *sampleData, int sampleLength) {
    if (decoder != NULL) {
        int decoded_samples = opus_multistream_decode(decoder, (unsigned char *) sampleData, sampleLength,
                                                      (opus_int16 *) buffer, frame_size, 0);
        // Encode the decoded audio to AAC
        INT inBufId = IN_AUDIO_DATA, outBufId = OUT_BITSTREAM_DATA;
        INT inBufSize = unit_size * decoded_samples, outBufSize = unit_size * frame_size;
        INT outBufElSize = 1;
        AACENC_BufDesc in_buf = {0}, out_buf = {0};
        AACENC_InArgs in_args = {0};
        AACENC_OutArgs out_args = {0};

        in_buf.numBufs = 1;
        in_buf.bufs = (void **) &buffer;
        in_buf.bufferIdentifiers = &inBufId;
        in_buf.bufSizes = &inBufSize;
        in_buf.bufElSizes = &unit_size;

        out_buf.numBufs = 1;
        out_buf.bufs = (void **) &buffer2;
        out_buf.bufferIdentifiers = &outBufId;
        out_buf.bufSizes = &outBufSize;
        out_buf.bufElSizes = &outBufElSize;

        in_args.numInSamples = decoded_samples * channel_count;

        AACENC_ERROR encError = aacEncEncode(aac_encoder, &in_buf, &out_buf, &in_args, &out_args);
        if (encError != AACENC_OK) {
            commons_log_error("Audio", "Failed to encode audio: %d", encError);
            return;
        }
        if (out_args.numOutBytes > 0) {
            SS4S_PlayerAudioFeed(player, buffer2, out_args.numOutBytes);
        }
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
    }
    header->mappingTable.streamCount = opusConfig->streams;
    header->mappingTable.coupledCount = opusConfig->coupledStreams;
    for (int i = 0; i < opusConfig->streams; i++) {
        header->mappingTable.mapping[i] = opusConfig->mapping[i];
    }
    return 21 + opusConfig->streams;
}

AUDIO_RENDERER_CALLBACKS ss4s_aud_callbacks = {
        .init = aud_init,
        .cleanup = aud_cleanup,
        .decodeAndPlaySample = aud_feed,
        .capabilities = CAPABILITY_DIRECT_SUBMIT,
};