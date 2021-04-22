#include "ndl_common.h"
#include "stream/module/api.h"
#include "util/logging.h"

#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Limelight.h>
#include <NDL_directmedia.h>

// 2MB decode size should be fairly enough for everything
#define DECODER_BUFFER_SIZE 2048 * 1024

#define decoder_callbacks PLUGIN_SYMBOL_NAME(decoder_callbacks)

static char *ndl_buffer;

static int ndl_setup(int videoFormat, int width, int height, int redrawRate, void *context, int drFlags)
{
#if NDL_WEBOS5
  switch (videoFormat)
  {
  case VIDEO_FORMAT_H264:
    media_info.video.type = NDL_VIDEO_TYPE_H264;
    break;
  case VIDEO_FORMAT_H265:
  case VIDEO_FORMAT_H265_MAIN10:
    media_info.video.type = NDL_VIDEO_TYPE_H265;
    break;
  default:
    return ERROR_UNKNOWN_CODEC;
  }
  media_info.video.width = width;
  media_info.video.height = height;
  media_info.video.unknown1 = 0;
  if (media_reload() != 0)
  {
    applog_e("NDL", "NDL_DirectMediaLoad failed: %s", NDL_DirectMediaGetError());
    return ERROR_DECODER_OPEN_FAILED;
  }
#else
  NDL_DIRECTVIDEO_DATA_INFO info = {width, height};
  if (NDL_DirectVideoOpen(&info) != 0)
  {
    applog_e("NDL", "NDL_DirectVideoOpen failed: %s", NDL_DirectMediaGetError());
    return ERROR_DECODER_OPEN_FAILED;
  }
  applog_d("NDL", "NDL_DirectVideoOpen %d * %d", width, height);
#endif
  ndl_buffer = malloc(DECODER_BUFFER_SIZE);
  if (ndl_buffer == NULL)
  {
    applog_e("NDL", "Not enough memory");
#if NDL_WEBOS5
    media_unload();
#else
    NDL_DirectVideoClose();
#endif
    return ERROR_OUT_OF_MEMORY;
  }
  return 0;
}

static void ndl_cleanup()
{
#if NDL_WEBOS5
  media_unload();
  memset(&media_info.video, 0, sizeof(media_info.video));
#else
  NDL_DirectVideoClose();
#endif
  if (ndl_buffer)
  {
    free(ndl_buffer);
  }
}

static int ndl_submit_decode_unit(PDECODE_UNIT decodeUnit)
{
#if NDL_WEBOS5
  if (!media_loaded)
    return DR_NEED_IDR;
#endif
  if (decodeUnit->fullLength < DECODER_BUFFER_SIZE)
  {
    int length = 0;
    for (PLENTRY entry = decodeUnit->bufferList; entry != NULL; entry = entry->next)
    {
      memcpy(ndl_buffer + length, entry->data, entry->length);
      length += entry->length;
    }

#if NDL_WEBOS5
    if (NDL_DirectVideoPlay(ndl_buffer, length, 0) != 0)
#else
    if (NDL_DirectVideoPlay(ndl_buffer, length) != 0)
#endif
    {
      applog_w("NDL", "NDL_DirectVideoPlay failed");
      return DR_NEED_IDR;
    }
  }
  else
  {
    applog_w("NDL", "Video decode buffer too small, skip this frame");
    return DR_NEED_IDR;
  }

  return DR_OK;
}

DECODER_RENDERER_CALLBACKS decoder_callbacks = {
    .setup = ndl_setup,
    .cleanup = ndl_cleanup,
    .submitDecodeUnit = ndl_submit_decode_unit,
    .capabilities = CAPABILITY_SLICES_PER_FRAME(4) | CAPABILITY_DIRECT_SUBMIT,
};
