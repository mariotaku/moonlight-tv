#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <lgnc_system.h>
#include <lgnc_directvideo.h>
#include "stream/module/api.h"
#include "util/logging.h"

static bool lgnc_initialized = false;
logvprintf_fn module_logvprintf;

bool decoder_init_lgnc(int argc, char *argv[], PHOST_CONTEXT hctx)
{
    if (hctx)
    {
        module_logvprintf = hctx->logvprintf;
    }
#ifdef DECODER_LGNC_NOINIT
    lgnc_initialized = true;
#else
    LGNC_SYSTEM_CALLBACKS_T callbacks = {
        .pfnJoystickEventCallback = NULL,
        .pfnMsgHandler = NULL,
        .pfnKeyEventCallback = NULL,
        .pfnMouseEventCallback = NULL,
    };
    applog_d("LGNC", "LGNC_SYSTEM_Initialize");
    if (LGNC_SYSTEM_Initialize(argc, argv, &callbacks) == 0)
    {
        lgnc_initialized = true;
        applog_i("LGNC", "Initialized");
    }
    else
    {
        lgnc_initialized = false;
        applog_e("LGNC", "Unable to initialize LGNC");
    }
#endif
    return lgnc_initialized;
}

bool decoder_check_lgnc(PDECODER_INFO dinfo)
{
    applog_d("LGNC", "Initialize decoder info");
    dinfo->valid = true;
    dinfo->accelerated = true;
    dinfo->audio = true;
    dinfo->maxBitrate = 40000;
    return true;
}
void decoder_finalize_lgnc()
{
    if (lgnc_initialized)
    {
#ifndef DECODER_LGNC_NOINIT
        LGNC_SYSTEM_Finalize();
        applog_d("LGNC", "LGNC_SYSTEM_Finalize");
#endif
        lgnc_initialized = false;
    }
}