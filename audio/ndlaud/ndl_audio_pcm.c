#include "stream/module/api.h"
#include "util/logging.h"

#include <NDL_directmedia.h>

#include <stdio.h>
#include <opus_multistream.h>

#define MAX_CHANNEL_COUNT 8
#define FRAME_SIZE 240

static OpusMSDecoder *decoder;
static short pcmBuffer[FRAME_SIZE * MAX_CHANNEL_COUNT];
static int channelCount;

static int ndl_renderer_init(int audioConfiguration, POPUS_MULTISTREAM_CONFIGURATION opusConfig, void *context, int arFlags)
{
  int rc;
  decoder = opus_multistream_decoder_create(opusConfig->sampleRate, opusConfig->channelCount, opusConfig->streams,
                                            opusConfig->coupledStreams, opusConfig->mapping, &rc);

  channelCount = opusConfig->channelCount;

  NDL_DIRECTAUDIO_DATA_INFO info = {
      .numChannel = channelCount,
      .bitPerSample = 16,
      .nodelay = 1,
      .upperThreshold = 48,
      .lowerThreshold = 16,
      .channel = NDL_DIRECTAUDIO_CH_MAIN,
      .srcType = NDL_DIRECTAUDIO_SRC_TYPE_PCM,
      .samplingFreq = NDL_DIRECTAUDIO_SAMPLING_FREQ_OF(opusConfig->sampleRate)};

  if (NDL_DirectAudioOpen(&info) < 0)
  {
    applog_e("NDLAud", "Failed to open audio: %s", NDL_DirectMediaGetError());
    return -1;
  }
  applog_i("NDLAud", "NDL Audio opened with %d channels", info.numChannel);

  return 0;
}

static void ndl_renderer_cleanup()
{
  if (decoder != NULL)
    opus_multistream_decoder_destroy(decoder);

  NDL_DirectAudioClose();
}

static void ndl_renderer_decode_and_play_sample(char *data, int length)
{
  int decodeLen = opus_multistream_decode(decoder, data, length, pcmBuffer, FRAME_SIZE, 0);
  if (decodeLen > 0)
  {
#if NDL_WEBOS5
    NDL_DirectAudioPlay(pcmBuffer, decodeLen * channelCount * sizeof(short), 0);
#else
    NDL_DirectAudioPlay(pcmBuffer, decodeLen * channelCount * sizeof(short));
#endif
  }
  else
  {
    applog_e("NDLAud", "Opus error from decode: %d", decodeLen);
  }
}

AUDIO_RENDERER_CALLBACKS audio_callbacks_ndlaud_pcm = {
    .init = ndl_renderer_init,
    .cleanup = ndl_renderer_cleanup,
    .decodeAndPlaySample = ndl_renderer_decode_and_play_sample,
    .capabilities = CAPABILITY_DIRECT_SUBMIT,
};
