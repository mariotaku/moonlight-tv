#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <dlfcn.h>

#include "util/os_info.h"
#include "stream/module/api.h"
#include "media_services.h"

#define decoder_init PLUGIN_SYMBOL_NAME(decoder_init)
#define decoder_check PLUGIN_SYMBOL_NAME(decoder_check)
#define decoder_finalize PLUGIN_SYMBOL_NAME(decoder_finalize)

bool decoder_init(int argc, char *argv[])
{
    return true;
}

bool decoder_check(PDECODER_INFO dinfo)
{
    bool supported = media_services_supported();
    if (!supported)
        return false;
    dinfo->valid = true;
    dinfo->accelerated = true;
    dinfo->audio = true;
    dinfo->hevc = true;
    dinfo->colorSpace = COLORSPACE_REC_709;
    dinfo->maxBitrate = 50000;
    return true;
}

void decoder_finalize()
{
}
