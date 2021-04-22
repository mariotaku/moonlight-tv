#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <NDL_directmedia.h>

#define MODULE_IMPL
#include "ndl_common.h"
#include "stream/module/api.h"
#include "util/logging.h"

bool media_initialized = false;
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
        media_initialized = true;
    }
    else
    {
        media_initialized = false;
        applog_e("NDL", "Unable to initialize NDL: %s", NDL_DirectMediaGetError());
    }
#if NDL_WEBOS5
    memset(&media_info, 0, sizeof(media_info));
#endif
    return media_initialized;
}

bool decoder_check(PDECODER_INFO dinfo)
{
#ifndef NDL_WEBOS5
    // On webOS 5, loading video requires SDL window to be created. This can cause a lot trouble.
    // So we cheese it and assume it's supported.
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
    if (media_initialized)
    {
        NDL_DirectMediaQuit();
        media_initialized = false;
    }
}