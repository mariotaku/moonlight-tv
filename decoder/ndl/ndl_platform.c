#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <NDL_directmedia.h>

#define MODULE_IMPL
#include "ndl_common.h"
#include "stream/module/api.h"
#include "util/logging.h"

static bool ndl_initialized = false;
logvprintf_fn module_logvprintf;

#define decoder_init PLUGIN_SYMBOL_NAME(decoder_init)
#define decoder_check PLUGIN_SYMBOL_NAME(decoder_check)
#define decoder_finalize PLUGIN_SYMBOL_NAME(decoder_finalize)

bool decoder_init(int argc, char *argv[], PHOST_CONTEXT hctx)
{
    if (hctx)
    {
        module_logvprintf = hctx->logvprintf;
    }
    if (NDL_DirectMediaInit(getenv("APPID"), NULL) == 0)
    {
        ndl_initialized = true;
    }
    else
    {
        ndl_initialized = false;
        applog_e("NDL", "Unable to initialize NDL: %s", NDL_DirectMediaGetError());
    }
#if NDL_WEBOS5
    memset(&media_info, 0, sizeof(media_info));
#endif
    return ndl_initialized;
}

bool decoder_check(PDECODER_INFO dinfo)
{
#if NDL_WEBOS5
    for (int i = 0; i < 3; i++)
    {
        NDL_DIRECTMEDIA_DATA_INFO info = {
            .video.type = NDL_VIDEO_TYPE_H265,
            .video.width = 1270,
            .video.height = 720,
            .audio.type = NDL_AUDIO_TYPE_OPUS,
            .audio.opus.channels = 2,
            .audio.opus.sampleRate = 48.000,
            .audio.opus.streamHeader = NULL,
        };
        if (NDL_DirectMediaLoad(&info, media_load_callback) != 0)
        {
            applog_e("NDL", "NDL_DirectMediaLoad failed on attempt %d: %s", i, NDL_DirectMediaGetError());
            return false;
        }
        NDL_DirectMediaUnload();
    }
#else
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