#include <lgnc_directaudio.h>

#include <stdio.h>
#include <opus_multistream.h>

#include "stream/module/api.h"
#include "util/logging.h"

#define MAX_CHANNEL_COUNT 6
#define FRAME_SIZE 240

static OpusMSDecoder *decoder;
static short pcmBuffer[FRAME_SIZE * MAX_CHANNEL_COUNT];
static int channelCount;

static int lgnc_renderer_init(int audioConfiguration, POPUS_MULTISTREAM_CONFIGURATION opusConfig, void *context, int arFlags)
{
  int rc;
  decoder = opus_multistream_decoder_create(opusConfig->sampleRate, opusConfig->channelCount, opusConfig->streams, opusConfig->coupledStreams, opusConfig->mapping, &rc);

  channelCount = opusConfig->channelCount;

  LGNC_ADEC_DATA_INFO_T info = {
      .codec = LGNC_ADEC_FMT_PCM,
      .AChannel = LGNC_ADEC_CH_INDEX_MAIN,
      .samplingFreq = LGNC_ADEC_SAMPLING_FREQ_OF(opusConfig->sampleRate),
      .numberOfChannel = opusConfig->channelCount,
      .bitPerSample = 16,
  };
  applog_i("LGNC", "Opening audio, codec=%d, sampleRate=%d, numberOfChannel=%d", info.codec, info.samplingFreq, info.numberOfChannel);
  if (LGNC_DIRECTAUDIO_Open(&info) != 0)
  {
    applog_e("LGNC", "Failed to open audio");
    return ERROR_AUDIO_OPEN_FAILED;
  }

  return 0;
}

static void lgnc_renderer_cleanup()
{
  if (decoder != NULL)
    opus_multistream_decoder_destroy(decoder);

  LGNC_DIRECTAUDIO_Close();
}

static void lgnc_renderer_decode_and_play_sample(char *data, int length)
{
  int decodeLen = opus_multistream_decode(decoder, data, length, pcmBuffer, FRAME_SIZE, 0);
  if (decodeLen > 0)
  {
    LGNC_DIRECTAUDIO_Play(pcmBuffer, decodeLen * channelCount * sizeof(short));
  }
  else
  {
    applog_w("LGNC", "Opus error from decode: %d", decodeLen);
  }
}

AUDIO_RENDERER_CALLBACKS audio_callbacks_lgnc = {
    .init = lgnc_renderer_init,
    .cleanup = lgnc_renderer_cleanup,
    .decodeAndPlaySample = lgnc_renderer_decode_and_play_sample,
    .capabilities = CAPABILITY_DIRECT_SUBMIT,
};
