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

#define audio_init PLUGIN_SYMBOL_NAME(audio_init)
#define audio_check PLUGIN_SYMBOL_NAME(audio_check)
#define audio_finalize PLUGIN_SYMBOL_NAME(audio_finalize)

bool audio_init(int argc, char *argv[], PHOST_CONTEXT hctx)
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
        applog_e("NDLAud", "Unable to initialize NDL: %s", NDL_DirectMediaGetError());
    }
    memset(&media_info, 0, sizeof(media_info));
    return media_initialized;
}

bool audio_check(PAUDIO_INFO dinfo)
{
    dinfo->valid = true;
#if DEBUG
    dinfo->configuration = AUDIO_CONFIGURATION_51_SURROUND;
#endif
    return true;
}

void audio_finalize()
{
    if (media_initialized)
    {
        NDL_DirectMediaQuit();
        media_initialized = false;
    }
}