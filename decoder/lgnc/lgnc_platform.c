#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <lgnc_system.h>
#include <lgnc_directvideo.h>
#include "platform/webos/os_info.h"
#include "stream/platform.h"

static bool lgnc_initialized = false;

bool platform_init_lgnc(int argc, char *argv[])
{
    char webos_release[16];
    webos_os_info_get_release(webos_release, sizeof(webos_release));
    if (atoi(webos_release) > 4)
    {
        fprintf(stdout, "This webOS version doesn't support LGNC, skipping\n");
        return false;
    }
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
    return lgnc_initialized;
}

bool platform_check_lgnc(PPLATFORM_INFO platform_info)
{
    LGNC_VDEC_DATA_INFO_T info = {.width = 1270, .height = 720, .vdecFmt = LGNC_VDEC_FMT_H264, .trid_type = LGNC_VDEC_3D_TYPE_NONE};
    if (LGNC_DIRECTVIDEO_Open(&info) != 0)
        return false;
    LGNC_DIRECTVIDEO_Close();
    platform_info->valid = true;
    return true;
}
void platform_finalize_lgnc()
{
    if (lgnc_initialized)
    {
        LGNC_SYSTEM_Finalize();
        lgnc_initialized = false;
    }
}