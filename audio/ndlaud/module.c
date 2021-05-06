#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <NDL_directmedia.h>

#define MODULE_IMPL
#include "stream/module/api.h"
#include "util/logging.h"

static bool ndl_initialized = false;
logvprintf_fn module_logvprintf;

bool audio_init_ndlaud(int argc, char *argv[], PHOST_CONTEXT hctx)
{
    if (hctx)
    {
        module_logvprintf = hctx->logvprintf;
    }
    applog_d("NDLAud", "init");
    if (NDL_DirectMediaInit(getenv("APPID"), NULL) == 0)
    {
        ndl_initialized = true;
    }
    else
    {
        ndl_initialized = false;
        applog_e("NDLAud", "Unable to initialize NDL: %s", NDL_DirectMediaGetError());
    }
    return ndl_initialized;
}

bool audio_check_ndlaud(PAUDIO_INFO ainfo)
{
    ainfo->valid = true;
    return true;
}

void audio_finalize_ndlaud()
{
    if (ndl_initialized)
    {
        NDL_DirectMediaQuit();
        ndl_initialized = false;
    }
}