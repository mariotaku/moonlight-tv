#include <cgl.h>

#include <opus_multistream.h>

#include "stream/module/api.h"
#include "util/logging.h"

#define MAX_CHANNEL_COUNT 6
#define FRAME_SIZE 240

static OpusMSDecoder *decoder;
static short pcmBuffer[FRAME_SIZE * MAX_CHANNEL_COUNT];
static int channelCount;

static int aud_init(int audioConfiguration, POPUS_MULTISTREAM_CONFIGURATION opusConfig, void *context, int arFlags) {
    int rc;
    decoder = opus_multistream_decoder_create(opusConfig->sampleRate, opusConfig->channelCount, opusConfig->streams,
                                              opusConfig->coupledStreams, opusConfig->mapping, &rc);

    channelCount = opusConfig->channelCount;

    CGL_AUDIO_INFO_T info = {
            .codec = CGL_AUDIO_FMT_PCM,
            .channel = CGL_AUDIO_CH_INDEX_MAIN,
            .samplingFreq = CGL_AUDIO_SAMPLING_FREQ_OF(opusConfig->sampleRate),
            .numberOfChannel = opusConfig->channelCount,
            .bitPerSample = 16,
    };
    applog_i("CGL", "Opening audio, codec=%d, sampleRate=%d, numberOfChannel=%d", info.codec, info.samplingFreq,
             info.numberOfChannel);
    if (CGL_OpenAudio(&info) != 0) {
        applog_e("CGL", "Failed to open audio");
        return ERROR_AUDIO_OPEN_FAILED;
    }

    return 0;
}

static void aud_cleanup() {
    if (decoder != NULL)
        opus_multistream_decoder_destroy(decoder);

    CGL_CloseAudio();
}

static void aud_submit(char *data, int length) {
    int decodeLen = opus_multistream_decode(decoder, data, length, pcmBuffer, FRAME_SIZE, 0);
    if (decodeLen > 0) {
        CGL_PlayAudio(pcmBuffer, decodeLen * channelCount * sizeof(short));
    } else {
        applog_w("CGL", "Opus error from decode: %d", decodeLen);
    }
}

MODULE_API AUDIO_RENDERER_CALLBACKS audio_callbacks_cgl = {
        .init = aud_init,
        .cleanup = aud_cleanup,
        .decodeAndPlaySample = aud_submit,
        .capabilities = CAPABILITY_DIRECT_SUBMIT,
};
