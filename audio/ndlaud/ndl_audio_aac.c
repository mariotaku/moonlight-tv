#include "stream/module/api.h"
#include "util/logging.h"

#include <NDL_directmedia.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <opus_multistream.h>
#include <aacenc_lib.h>

#define MAX_CHANNEL_COUNT 8

static int channelCount, samplesPerFrame;

static OpusMSDecoder *opus_dec = NULL;
static HANDLE_AACENCODER aac_enc = NULL;
static size_t pcmBufferLen;
static short *pcmBuffer;
static unsigned char aacBuffer[1024 * MAX_CHANNEL_COUNT];
static INT aacInBufferIds[1] = {IN_AUDIO_DATA}, aacInBufSizes[1], aacInBufferElSizes[1] = {sizeof(short)};
static INT aacOutBufferIds[1] = {OUT_BITSTREAM_DATA}, aacOutBufSizes[1] = {sizeof(aacBuffer)}, aacOutBufferElSizes[1] = {1};

static void *pcmBufs[1], *aacBufs[1];

static AACENC_BufDesc inBufDesc = {
    .numBufs = 1,
    .bufs = pcmBufs,
    .bufferIdentifiers = aacInBufferIds,
    .bufSizes = aacInBufSizes,
    .bufElSizes = aacInBufferElSizes,
};
static AACENC_BufDesc outBufDesc = {
    .numBufs = 1,
    .bufs = aacBufs,
    .bufferIdentifiers = aacOutBufferIds,
    .bufSizes = aacOutBufSizes,
    .bufElSizes = aacOutBufferElSizes,
};

static AACENC_InArgs inargs = {0, 0};
static AACENC_OutArgs outargs = {0};

static int ndl_renderer_init(int audioConfiguration, POPUS_MULTISTREAM_CONFIGURATION opusConfig, void *context, int arFlags)
{
  channelCount = opusConfig->channelCount;
  samplesPerFrame = opusConfig->samplesPerFrame;
  pcmBufferLen = samplesPerFrame * channelCount * 4;
  aacInBufSizes[0] = pcmBufferLen * sizeof(short);
  pcmBuffer = calloc(pcmBufferLen, sizeof(short));
  pcmBufs[0] = pcmBuffer;
  aacBufs[0] = aacBuffer;
  assert(pcmBuffer);
  inargs.numInSamples = 0;
  int rc;
  aacEncOpen(&aac_enc, 0, channelCount);
  aacEncoder_SetParam(aac_enc, AACENC_TRANSMUX, 2 /* ADTS format */);
  aacEncoder_SetParam(aac_enc, AACENC_AOT, 2 /*MPEG-4 AAC LC*/);
  aacEncoder_SetParam(aac_enc, AACENC_SAMPLERATE, opusConfig->sampleRate);
  unsigned char channelMapping[MAX_CHANNEL_COUNT];
  switch (audioConfiguration)
  {
  case AUDIO_CONFIGURATION_STEREO:
    aacEncoder_SetParam(aac_enc, AACENC_CHANNELMODE, MODE_2);
    aacEncoder_SetParam(aac_enc, AACENC_BITRATE, 256000 /* 256kbps */);
    channelMapping[0] = opusConfig->mapping[0]; // Left
    channelMapping[1] = opusConfig->mapping[1]; // Right
    break;
  case AUDIO_CONFIGURATION_51_SURROUND:
    aacEncoder_SetParam(aac_enc, AACENC_CHANNELMODE, MODE_1_2_2_1);
    aacEncoder_SetParam(aac_enc, AACENC_BITRATE, 320000 /* 320kbps */);
    channelMapping[0] = opusConfig->mapping[2]; // Center
    channelMapping[1] = opusConfig->mapping[0]; // Front Left
    channelMapping[2] = opusConfig->mapping[1]; // Front Right
    channelMapping[3] = opusConfig->mapping[4]; // Surround Left
    channelMapping[4] = opusConfig->mapping[5]; // Surround Right
    channelMapping[5] = opusConfig->mapping[3]; // LFE
    break;
  case AUDIO_CONFIGURATION_71_SURROUND:
    aacEncoder_SetParam(aac_enc, AACENC_CHANNELMODE, MODE_7_1_BACK);
    aacEncoder_SetParam(aac_enc, AACENC_BITRATE, 512000 /* 512kbps */);
    channelMapping[0] = opusConfig->mapping[2]; // Center
    channelMapping[1] = opusConfig->mapping[0]; // Front Left
    channelMapping[2] = opusConfig->mapping[1]; // Front Right
    channelMapping[3] = opusConfig->mapping[4]; // Surround Left
    channelMapping[4] = opusConfig->mapping[5]; // Surround Right
    channelMapping[5] = opusConfig->mapping[6]; // Side Left
    channelMapping[6] = opusConfig->mapping[7]; // Side Right
    channelMapping[7] = opusConfig->mapping[3]; // LFE
    break;
  default:
    return -1;
  }

  AACENC_ERROR aacErr;
  if ((aacErr = aacEncEncode(aac_enc, NULL, NULL, NULL, NULL)) != AACENC_OK)
  {
    applog_e("NDLAud", "aacEncEncode() failed: %x", aacErr);
    return -1;
  }
  AACENC_InfoStruct encInfo;
  if ((aacErr = aacEncInfo(aac_enc, &encInfo)) != AACENC_OK)
  {
    applog_e("NDLAud", "aacEncInfo() failed: %x", aacErr);
    return -1;
  }

  opus_dec = opus_multistream_decoder_create(opusConfig->sampleRate, opusConfig->channelCount, opusConfig->streams,
                                             opusConfig->coupledStreams, channelMapping, &rc);

  NDL_DIRECTAUDIO_DATA_INFO info = {
      .numChannel = channelCount,
      .bitPerSample = 0,
      .nodelay = 1,
      .upperThreshold = 64,
      .lowerThreshold = 16,
      .channel = NDL_DIRECTAUDIO_CH_MAIN,
      .srcType = NDL_DIRECTAUDIO_SRC_TYPE_AAC,
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
  if (opus_dec != NULL)
    opus_multistream_decoder_destroy(opus_dec);

  if (aac_enc != NULL)
    aacEncClose(&aac_enc);
  NDL_DirectAudioClose();
  free(pcmBuffer);
}

static void ndl_renderer_decode_and_play_sample(char *data, int length)
{
  size_t decOffset = 0;
  int decodeLen = opus_multistream_decode(opus_dec, data, length, &pcmBuffer[inargs.numInSamples], samplesPerFrame, 0);
  if (decodeLen > 0)
  {
    inargs.numInSamples += decodeLen * channelCount;
    AACENC_ERROR err;
    if ((err = aacEncEncode(aac_enc, &inBufDesc, &outBufDesc, &inargs, &outargs)) != AACENC_OK)
    {
      applog_w("NDLAud", "Failed to encode AAC: 0x%x", err);
      return;
    }
    if (outargs.numInSamples > 0)
    {
      FDKmemmove(pcmBuffer, &pcmBuffer[outargs.numInSamples], sizeof(short) * (inargs.numInSamples - outargs.numInSamples));
      inargs.numInSamples -= outargs.numInSamples;
    }
#if NDL_WEBOS5
    if (outargs.numOutBytes > 0 && NDL_DirectAudioPlay(aacBuffer, outargs.numOutBytes, 0) != 0)
#else
    if (outargs.numOutBytes > 0 && NDL_DirectAudioPlay(aacBuffer, outargs.numOutBytes) != 0)
#endif
    {
      applog_w("NDLAud", "Failed to play AAC: 0x%x", err);
      return;
    }
  }
  else
  {
    applog_e("NDLAud", "Opus error from decode: %d", decodeLen);
  }
}

AUDIO_RENDERER_CALLBACKS audio_callbacks_ndlaud_aac = {
    .init = ndl_renderer_init,
    .cleanup = ndl_renderer_cleanup,
    .decodeAndPlaySample = ndl_renderer_decode_and_play_sample,
    .capabilities = CAPABILITY_DIRECT_SUBMIT,
};
