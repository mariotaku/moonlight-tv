#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <NDL_directmedia.h>
#include "stream/platform.h"

static bool ndl_initialized = false;

bool platform_init_ndl(int argc, char *argv[])
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
    return ndl_initialized;
}

bool platform_check_ndl(PPLATFORM_INFO pinfo)
{
    NDL_DIRECTVIDEO_DATA_INFO info = {.width = 1270, .height = 720};
    if (NDL_DirectVideoOpen(&info) != 0)
        return false;
    NDL_DirectVideoClose();
    pinfo->valid = true;
    pinfo->vrank = 10;
    pinfo->arank = 30;
    pinfo->maxBitrate = 50000;
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