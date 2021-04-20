#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <NDL_directmedia.h>
#include "stream/api.h"
#include "ndl_common.h"

static bool ndl_initialized = false;

#define decoder_init PLUGIN_SYMBOL_NAME(decoder_init)
#define decoder_check PLUGIN_SYMBOL_NAME(decoder_check)
#define decoder_finalize PLUGIN_SYMBOL_NAME(decoder_finalize)

bool decoder_init(int argc, char *argv[])
{
    if (NDL_DirectMediaInit(getenv("APPID"), NULL) == 0)
    {
        ndl_initialized = true;
    }
    else
    {
        ndl_initialized = false;
        fprintf(stderr, "Unable to initialize NDL: %s\n", NDL_DirectMediaGetError());
    }
#if NDL_WEBOS5
    memset(&media_info, 0, sizeof(media_info));
#endif
    return ndl_initialized;
}

bool decoder_check(PDECODER_INFO dinfo)
{
#ifndef NDL_WEBOS5
    NDL_DIRECTVIDEO_DATA_INFO info = {.width = 1270, .height = 720};
    if (NDL_DirectVideoOpen(&info) != 0)
        return false;
    NDL_DirectVideoClose();
#endif
    dinfo->valid = true;
    dinfo->accelerated = true;
    dinfo->audio = true;
#if NDL_WEBOS5
    dinfo->hevc = true;
#endif
    dinfo->colorSpace = COLORSPACE_REC_709;
    dinfo->maxBitrate = 50000;
    return true;
}

void decoder_finalize()
{
    if (ndl_initialized)
    {
        NDL_DirectMediaQuit();
        ndl_initialized = false;
    }
}