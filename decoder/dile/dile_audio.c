#include "audio.h"

#include <dile_audio_direct.h>

#include <stdio.h>
#include <opus_multistream.h>

#include "utils.h"

static OpusMSDecoder *decoder;
static short pcmBuffer[FRAME_SIZE * MAX_CHANNEL_COUNT];
static int channelCount;

static int _init(int audioConfiguration, POPUS_MULTISTREAM_CONFIGURATION opusConfig, void *context, int arFlags)
{
  int rc;
  decoder = opus_multistream_decoder_create(opusConfig->sampleRate, opusConfig->channelCount, opusConfig->streams, opusConfig->coupledStreams, opusConfig->mapping, &rc);

  channelCount = opusConfig->channelCount;

  if (DILE_AUDIO_DIRECT_Open(0, 0) != 0)
  {
    printf("Failed to open audio\n");
    return -1;
  }
  if (DILE_AUDIO_DIRECT_Start(0, DILE_AUDIO_DIRECT_SRCTYPE_PCM, DILE_AUDIO_DIRECT_SAMPFREQ_OF(opusConfig->sampleRate),
                              opusConfig->channelCount, 16) != 0)
  {
    printf("Failed to start audio\n");
    DILE_AUDIO_DIRECT_Close(0);
    return -1;
  }

  return 0;
}

static void _cleanup()
{
  if (decoder != NULL)
    opus_multistream_decoder_destroy(decoder);

  DILE_AUDIO_DIRECT_Stop(0);
  DILE_AUDIO_DIRECT_Close(0);
}

static void _feed(char *data, int length)
{
  int decodeLen = opus_multistream_decode(decoder, data, length, pcmBuffer, FRAME_SIZE, 0);
  if (decodeLen > 0)
  {
    DILE_AUDIO_DIRECT_Write(0, pcmBuffer, decodeLen * channelCount * sizeof(short));
  }
  else
  {
    printf("Opus error from decode: %d\n", decodeLen);
  }
}

AUDIO_RENDERER_CALLBACKS audio_callbacks_dile = {
    .init = _init,
    .cleanup = _cleanup,
    .decodeAndPlaySample = _feed,
    .capabilities = CAPABILITY_DIRECT_SUBMIT,
};

bool policyActionHandler(const char *action, const char *resources,
                         const char *requestor_type, const char *requestor_name,
                         const char *connection_id)
{
  printf("POLICY_ACTION: action=%s, resources=%s, requestor_type=%s,"
         "requestor_name=%s, connection_id=%s\n",
         action, resources, requestor_type, requestor_name, connection_id);
  return true;
}
