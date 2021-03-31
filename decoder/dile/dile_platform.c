#include "dile_platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>

#include "platform/webos/os_info.h"

static bool dile_initialized = false;
int dile_webos_version = 0;

void RTD_Log(int level, int b, char *fmt, ...)
{
    printf("[RTD_Log %d %08x] ", level, b);
    va_list arglist;
    va_start(arglist, fmt);
    vprintf(fmt, arglist);
    va_end(arglist);
}

bool platform_init_dile(int argc, char *argv[])
{
    char webos_release[16];
    webos_os_info_get_release(webos_release, sizeof(webos_release));
    dile_webos_version = atoi(webos_release);
    // if (dile_webos_version != 5)
    // {
    //     fprintf(stdout, "This webOS version doesn't support DILE, skipping\n");
    //     return false;
    // }

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

bool platform_check_dile()
{
    return true;
}

void platform_finalize_dile()
{
    if (dile_initialized)
    {
        dile_initialized = false;
    }
}
