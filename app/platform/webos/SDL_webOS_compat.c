#include <SDL_webOS.h>
#include <dlfcn.h>

SDL_bool SDL_webOSCursorVisibility(SDL_bool visible)
{
    SDL_bool (*fn)(SDL_bool visible) = dlsym(RTLD_NEXT, "SDL_webOSCursorVisibility");
    if (!fn)
        return SDL_FALSE;
    return fn(visible);
}

SDL_bool SDL_webOSGetPanelResolution(int *width, int *height)
{
    SDL_bool (*fn)(int *, int *) = dlsym(RTLD_NEXT, "SDL_webOSGetPanelResolution");
    if (!fn)
        return SDL_FALSE;
    return fn(width, height);
}

SDL_bool SDL_webOSGetRefreshRate(int *rate)
{
    SDL_bool (*fn)(int *) = dlsym(RTLD_NEXT, "SDL_webOSGetRefreshRate");
    if (!fn)
        return SDL_FALSE;
    return fn(rate);
}