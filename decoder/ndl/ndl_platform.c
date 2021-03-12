#include <stdio.h>
#include <stdlib.h>

#include <NDL_directmedia.h>

void platform_init_ndl(int argc, char *argv[])
{
    if (NDL_DirectMediaInit(getenv("APPID"), NULL) == 0)
    {
    }
    else
    {
        fprintf(stderr, "Unable to initialize NDL: %s\n", NDL_DirectMediaGetError());
    }
}

void platform_finalize_ndl()
{
    NDL_DirectMediaQuit();
}