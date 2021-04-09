#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <NDL_directmedia.h>
#include "platform/webos/os_info.h"
#include "stream/platform.h"

static bool ndl_initialized = false;

bool platform_init_ndl(int argc, char *argv[])
{
    char webos_release[16];
    webos_os_info_get_release(webos_release, sizeof(webos_release));
    if (NDL_DirectMediaInit(getenv("APPID"), NULL) == 0)
    {
        ndl_initialized = true;
    }
    else
    {
        ndl_initialized = false;
        fprintf(stderr, "Unable to initialize NDL: %s\n", NDL_DirectMediaGetError());
    }
    return ndl_initialized;
}

bool platform_check_ndl(PPLATFORM_INFO platform_info)
{
    NDL_DIRECTVIDEO_DATA_INFO info = {.width = 1270, .height = 720};
    if (NDL_DirectVideoOpen(&info) != 0)
        return false;
    NDL_DirectVideoClose();
    platform_info->valid = true;
    platform_info->hwaccel = true;
    return true;
}

void platform_finalize_ndl()
{
    if (ndl_initialized)
    {
        NDL_DirectMediaQuit();
        ndl_initialized = false;
    }
}