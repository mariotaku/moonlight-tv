#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <dlfcn.h>

#include "platform/webos/os_info.h"
#include "stream/platform.h"
#include "vdec_services.h"

bool DECODER_SYMBOL_NAME(platform_init)(int argc, char *argv[])
{
    return true;
}

bool DECODER_SYMBOL_NAME(platform_check)(PPLATFORM_INFO pinfo)
{
    bool supported = vdec_services_supported();
    if (!supported)
        return false;
    pinfo->valid = true;
    pinfo->vrank = 35;
    pinfo->arank = 0;
    pinfo->hevc = true;
    pinfo->maxBitrate = 50000;
    return true;
}

void DECODER_SYMBOL_NAME(platform_finalize)()
{
}
