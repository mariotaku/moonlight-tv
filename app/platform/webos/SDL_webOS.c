#include "SDL_webOS.h"

#include <dlfcn.h>

int SDL_webOSCursorVisibility(int visible)
{
    int (*hnd)(int) = dlsym(RTLD_NEXT, "SDL_webOSCursorVisibility");
    if (!hnd)
    {
        return -1;
    }
    return hnd(visible);
}