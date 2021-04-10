#include "audio.h"

#include <dile_audio_direct.h>

#include <stdio.h>
#include <opus_multistream.h>

static OpusMSDecoder *decoder;
static short pcmBuffer[FRAME_SIZE * MAX_CHANNEL_COUNT];
static int channelCount;

static int _init(int audioConfiguration, POPUS_MULTISTREAM_CONFIGURATION opusConfig, void *context, int arFlags)
{
  int rc;
  decoder = opus_multistream_decoder_create(opusConfig->sampleRate, opusConfig->channelCount, opusConfig->streams, opusConfig->coupledStreams, opusConfig->mapping, &rc);

  channelCount = opusConfig->channelCount;
  
  if (DILE_AUDIO_DIRECT_Open(0, 0) < 0)
  {
    printf("Failed to open audio: %s\n", NDL_DirectMediaGetError());
    return -1;
  }

  return 0;
}

static void _cleanup()
{
  if (decoder != NULL)
    opus_multistream_decoder_destroy(decoder);

  NDL_DirectAudioClose();
}

static void _decode_and_play_sample(char *data, int length)
{
  int decodeLen = opus_multistream_decode(decoder, data, length, pcmBuffer, FRAME_SIZE, 0);
  if (decodeLen > 0)
  {
    NDL_DirectAudioPlay(pcmBuffer, decodeLen * channelCount * sizeof(short));
  }
  else
  {
    printf("Opus error from decode: %d\n", decodeLen);
  }
}

AUDIO_RENDERER_CALLBACKS audio_callbacks_dile = {
    .init = _init,
    .cleanup = _cleanup,
    .decodeAndPlaySample = _decode_and_play_sample,
    .capabilities = CAPABILITY_DIRECT_SUBMIT,
};
