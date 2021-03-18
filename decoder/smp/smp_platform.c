#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

static bool smp_initialized = false;


bool platform_init_smp(int argc, char *argv[])
{
    // setenv("GST_DEBUG", "4", 1);
    // setenv("SDL_WEBOS_DEBUG", "1", 1);
    smp_initialized = true;
    return smp_initialized;
}

bool platform_check_smp()
{
    return true;
}

void platform_finalize_smp()
{
}