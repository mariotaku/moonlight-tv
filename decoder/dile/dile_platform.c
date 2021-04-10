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

bool DECODER_SYMBOL_NAME(platform_check)(PPLATFORM_INFO platform_info)
{
    bool supported = vdec_services_supported();
    if (!supported)
        return false;
    platform_info->valid = true;
    platform_info->audio = true;
    platform_info->hwaccel = true;
    platform_info->hevc = true;
    platform_info->maxBitrate = 50000;
    return true;
}

void DECODER_SYMBOL_NAME(platform_finalize)()
{
}
