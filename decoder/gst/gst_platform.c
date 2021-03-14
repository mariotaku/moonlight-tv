#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <gst/gst.h>

static bool gst_initialized = false;

bool platform_init_gst(int argc, char *argv[])
{
    setenv("GST_DEBUG", "3", 1);
    gst_init(&argc, &argv);
    gst_initialized = true;
    return gst_initialized;
}

bool platform_check_gst()
{
    return true;
}

void platform_finalize_gst()
{
}