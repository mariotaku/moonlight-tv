#include "SDL_webOS.h"

#include <dlfcn.h>

SDL_bool SDL_webOSCursorVisibility(SDL_bool visible)
{
    SDL_bool (*hnd)(SDL_bool) = dlsym(RTLD_NEXT, "SDL_webOSCursorVisibility");
    if (!hnd)
    {
        return SDL_FALSE;
    }
    return hnd(visible);
}

SDL_bool SDL_webOSGetPanelResolution(int *width, int *height)
{
    SDL_bool (*hnd)(int *, int *) = dlsym(RTLD_NEXT, "SDL_webOSGetPanelResolution");
    if (!hnd)
    {
        return SDL_FALSE;
    }
    return hnd(width, height);
}

SDL_bool SDL_webOSGetRefreshRate(int *rate)
{
    SDL_bool (*hnd)(int *) = dlsym(RTLD_NEXT, "SDL_webOSGetRefreshRate");
    if (!hnd)
    {
        return SDL_FALSE;
    }
    return hnd(rate);
}