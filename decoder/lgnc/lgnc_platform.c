#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <lgnc_system.h>
#include <lgnc_directvideo.h>
#include "stream/api.h"

static bool lgnc_initialized = false;

bool decoder_init_lgnc(int argc, char *argv[])
{
#if DECODER_LGNC_NOINIT
    lgnc_initialized = true;
#else
    LGNC_SYSTEM_CALLBACKS_T callbacks = {
        .pfnJoystickEventCallback = NULL,
        .pfnMsgHandler = NULL,
        .pfnKeyEventCallback = NULL,
        .pfnMouseEventCallback = NULL,
    };
    if (LGNC_SYSTEM_Initialize(argc, argv, &callbacks) == 0)
    {
        lgnc_initialized = true;
    }
    else
    {
        lgnc_initialized = false;
        fprintf(stderr, "Unable to initialize LGNC\n");
    }
#endif
    return lgnc_initialized;
}

bool decoder_check_lgnc(PDECODER_INFO dinfo)
{
    LGNC_VDEC_DATA_INFO_T info = {.width = 1270, .height = 720, .vdecFmt = LGNC_VDEC_FMT_H264, .trid_type = LGNC_VDEC_3D_TYPE_NONE};
    if (LGNC_DIRECTVIDEO_Open(&info) != 0)
        return false;
    LGNC_DIRECTVIDEO_Close();
    dinfo->valid = true;
    dinfo->accelerated = true;
    dinfo->audio= true;
    dinfo->maxBitrate = 40000;
    return true;
}
void decoder_finalize_lgnc()
{
    if (lgnc_initialized)
    {
#if !DECODER_LGNC_NOINIT
        LGNC_SYSTEM_Finalize();
#endif
        lgnc_initialized = false;
    }
}