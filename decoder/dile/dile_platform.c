#define _GNU_SOURCE
#include "dile_platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <dlfcn.h>

#include "platform/webos/os_info.h"
#include "vdec_services.h"

static bool dile_initialized = false;
int dile_webos_version = 0;

// void RTD_Log(int level, int b, char *fmt, ...)
// {
//     printf("[RTD_Log %d %08x] ", level, b);
//     va_list arglist;
//     va_start(arglist, fmt);
//     vprintf(fmt, arglist);
//     va_end(arglist);
// }

bool DECODER_SYMBOL_NAME(platform_init)(int argc, char *argv[])
{
    char webos_release[16];
    webos_os_info_get_release(webos_release, sizeof(webos_release));
    dile_webos_version = atoi(webos_release);
    if (true)
    {
        dile_initialized = true;
    }
    else
    {
        dile_initialized = false;
        fprintf(stderr, "Unable to initialize DILE, LSRegister error\n");
    }
    return dile_initialized;
}

bool DECODER_SYMBOL_NAME(platform_check)()
{
    return vdec_services_supported();
}

void DECODER_SYMBOL_NAME(platform_finalize)()
{
    if (dile_initialized)
    {
        dile_initialized = false;
    }
}
