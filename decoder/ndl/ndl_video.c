#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Limelight.h>
#include <NDL_directmedia.h>

// 2MB decode size should be fairly enough for everything
#define DECODER_BUFFER_SIZE 2048 * 1024

static char *ndl_buffer;

static int ndl_setup(int videoFormat, int width, int height, int redrawRate, void *context, int drFlags)
{
  NDL_DIRECTVIDEO_DATA_INFO info = {width, height};
  if (NDL_DirectVideoOpen(&info) < 0)
  {
    fprintf(stderr, "Couldn't initialize video decoding\n");
    return -1;
  }
  ndl_buffer = malloc(DECODER_BUFFER_SIZE);
  if (ndl_buffer == NULL)
  {
    fprintf(stderr, "Not enough memory\n");
    NDL_DirectVideoClose();
    return -1;
  }

  return 0;
}

static void ndl_cleanup()
{
  NDL_DirectVideoClose();
  if (ndl_buffer)
  {
    free(ndl_buffer);
  }
}

static int ndl_submit_decode_unit(PDECODE_UNIT decodeUnit)
{
  if (decodeUnit->fullLength < DECODER_BUFFER_SIZE)
  {
    int length = 0;
    for (PLENTRY entry = decodeUnit->bufferList; entry != NULL; entry = entry->next)
    {
      memcpy(ndl_buffer + length, entry->data, entry->length);
      length += entry->length;
    }
    if (NDL_DirectVideoPlay(ndl_buffer, length) == -1)
    {
      fprintf(stderr, "NDL_DirectVideoPlay returned -1\n");
    }
  }
  else
  {
    fprintf(stderr, "Video decode buffer too small, skip this frame");
    return DR_NEED_IDR;
  }

  return DR_OK;
}

DECODER_RENDERER_CALLBACKS decoder_callbacks_ndl = {
    .setup = ndl_setup,
    .cleanup = ndl_cleanup,
    .submitDecodeUnit = ndl_submit_decode_unit,
    .capabilities = CAPABILITY_SLICES_PER_FRAME(4) | CAPABILITY_DIRECT_SUBMIT,
};
