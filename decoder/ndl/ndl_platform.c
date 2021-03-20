#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <NDL_directmedia.h>
#include "platform/webos/os_info.h"

static bool ndl_initialized = false;

bool platform_init_ndl(int argc, char *argv[])
{
    char webos_release[16];
    webos_os_info_get_release(webos_release, sizeof(webos_release));
    if (atoi(webos_release) != 4)
    {
        fprintf(stdout, "This webOS version doesn't support NDL, skipping\n");
        return false;
    }
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

bool platform_check_ndl()
{
    NDL_DIRECTVIDEO_DATA_INFO info = {.width = 1270, .height = 720};
    if (NDL_DirectVideoOpen(&info) != 0)
        return false;
    NDL_DirectVideoClose();
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