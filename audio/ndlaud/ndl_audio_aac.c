/*
 * This file is part of Moonlight Embedded.
 *
 * Copyright (C) 2015-2017 Iwan Timmer
 *
 * Moonlight is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Moonlight is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Moonlight; if not, see <http://www.gnu.org/licenses/>.
 */

#include "stream/module/api.h"
#include "util/logging.h"

#include <NDL_directmedia.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <opus_multistream.h>
#include <aacenc_lib.h>

#define MAX_CHANNEL_COUNT 8
#define FRAME_SIZE 240

static int channelCount;

static OpusMSDecoder *opus_dec;
static HANDLE_AACENCODER aac_enc = NULL;
static size_t pcmBufferLen, pcmBufferResetThreshold;
static short *pcmBuffer;
static unsigned char aacBuffer[1024 * MAX_CHANNEL_COUNT];
static INT aacOutBufferIds[1] = {OUT_BITSTREAM_DATA}, aacOutBufSizes[1] = {sizeof(aacBuffer)}, aacOutBufferElSizes[1] = {1};
static INT aacInBufferIds[2] = {IN_AUDIO_DATA, IN_AUDIO_DATA}, aacInBufferElSizes[2] = {sizeof(short), sizeof(short)};

static int pcmLeftoverCursor = 0, pcmBufferCursor = 0;
static size_t pcmLeftoverSize = 0;

static int ndl_renderer_init(int audioConfiguration, POPUS_MULTISTREAM_CONFIGURATION opusConfig, void *context, int arFlags)
{
  channelCount = opusConfig->channelCount;
  pcmBufferLen = FRAME_SIZE * channelCount * 8;
  pcmBufferResetThreshold = FRAME_SIZE * channelCount * 6;
  pcmBuffer = calloc(pcmBufferLen, sizeof(short));
  assert(pcmBuffer);
  pcmLeftoverCursor = 0;
  pcmBufferCursor = 0;
  pcmLeftoverSize = 0;
  int rc;
  opus_dec = opus_multistream_decoder_create(opusConfig->sampleRate, opusConfig->channelCount, opusConfig->streams,
                                             opusConfig->coupledStreams, opusConfig->mapping, &rc);
  aacEncOpen(&aac_enc, 0, channelCount);
  aacEncoder_SetParam(aac_enc, AACENC_TRANSMUX, 2 /* ADTS format */);
  aacEncoder_SetParam(aac_enc, AACENC_AOT, 2 /*MPEG-4 AAC LC*/);
  aacEncoder_SetParam(aac_enc, AACENC_BITRATE, 262144 /* 256kbps */);
  aacEncoder_SetParam(aac_enc, AACENC_SAMPLERATE, opusConfig->sampleRate);
  switch (audioConfiguration)
  {
  case AUDIO_CONFIGURATION_STEREO:
    aacEncoder_SetParam(aac_enc, AACENC_CHANNELMODE, MODE_2);
    break;
  case AUDIO_CONFIGURATION_51_SURROUND:
    aacEncoder_SetParam(aac_enc, AACENC_CHANNELMODE, MODE_1_2_2_1);
    break;
  case AUDIO_CONFIGURATION_71_SURROUND:
    aacEncoder_SetParam(aac_enc, AACENC_CHANNELMODE, MODE_7_1_BACK);
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
  int decodeLen = opus_multistream_decode(opus_dec, data, length, &pcmBuffer[pcmBufferCursor], FRAME_SIZE, 0);
  if (decodeLen > 0)
  {
    int inNumBufs = 1;
    void *pcmBufs[2], *aacBufs[1] = {(void *)aacBuffer};
    INT inBufSizes[2];
    if (pcmLeftoverSize == 0)
    {
      // Just new sample
      inNumBufs = 1;
      pcmBufs[0] = (void *)&pcmBuffer[pcmBufferCursor];
      inBufSizes[0] = decodeLen * channelCount * sizeof(short);
    }
    else if (pcmBufferCursor < pcmLeftoverCursor)
    {
      // PCM cursor reset, feed leftover then new
      inNumBufs = 2;
      pcmBufs[0] = (void *)&pcmBuffer[pcmLeftoverCursor];
      inBufSizes[0] = pcmLeftoverSize * sizeof(short);
      pcmBufs[1] = (void *)&pcmBuffer[pcmBufferCursor];
      inBufSizes[1] = decodeLen * channelCount * sizeof(short);
    }
    else
    {
      // Feed PCM starts with leftover
      inNumBufs = 1;
      pcmBufs[0] = (void *)&pcmBuffer[pcmLeftoverCursor];
      inBufSizes[0] = (pcmLeftoverSize + decodeLen * channelCount) * sizeof(short);
    }
    AACENC_BufDesc inBufDesc = {
        .numBufs = inNumBufs,
        .bufs = pcmBufs,
        .bufferIdentifiers = aacInBufferIds,
        .bufSizes = inBufSizes,
        .bufElSizes = aacInBufferElSizes,
    };
    AACENC_BufDesc outBufDesc = {
        .numBufs = 1,
        .bufs = aacBufs,
        .bufferIdentifiers = aacOutBufferIds,
        .bufSizes = aacOutBufSizes,
        .bufElSizes = aacOutBufferElSizes,
    };
    AACENC_InArgs inargs = {pcmLeftoverSize + decodeLen * channelCount, 0};
    AACENC_OutArgs outargs = {0};
    AACENC_ERROR err;
    if ((err = aacEncEncode(aac_enc, &inBufDesc, &outBufDesc, &inargs, &outargs)) != AACENC_OK)
    {
      applog_w("NDLAud", "Failed to encode AAC: 0x%x", err);
      return;
    }
    // This is how many PCM samples left during this encode
    pcmLeftoverSize = inargs.numInSamples - outargs.numInSamples;
    assert(pcmLeftoverSize >= 0);
    if (pcmLeftoverSize)
    {
      // Next time when OPUS writes PCM data, start with tail of fresh data
      pcmBufferCursor += decodeLen * channelCount * sizeof(short);
      // This is the address where AAC should consume first
      pcmLeftoverCursor = pcmBufferCursor - pcmLeftoverSize;
    }
    // If no left over or OPUS is over 75% of the PCM buffer, reset position
    if (!pcmLeftoverSize || pcmBufferCursor > pcmBufferResetThreshold)
    {
      pcmBufferCursor = 0;
    }
#if NDL_WEBOS5
    if (outargs.numOutBytes && NDL_DirectAudioPlay(aacBuffer, outargs.numOutBytes, 0) != 0)
#else
    if (outargs.numOutBytes && NDL_DirectAudioPlay(aacBuffer, outargs.numOutBytes) != 0)
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

AUDIO_RENDERER_CALLBACKS audio_callbacks_ndlaud = {
    .init = ndl_renderer_init,
    .cleanup = ndl_renderer_cleanup,
    .decodeAndPlaySample = ndl_renderer_decode_and_play_sample,
    .capabilities = CAPABILITY_DIRECT_SUBMIT,
};
