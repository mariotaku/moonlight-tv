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

const char *SDL_webOSCreateExportedWindow(int type)
{
    char *(*hnd)(int) = dlsym(RTLD_NEXT, "SDL_webOSCreateExportedWindow");
    if (!hnd)
    {
        return NULL;
    }
    return hnd(type);
}

SDL_bool SDL_webOSSetExportedWindow(const char *windowId, SDL_Rect *src, SDL_Rect *dst)
{
    SDL_bool (*hnd)(const char *, SDL_Rect *, SDL_Rect *) = dlsym(RTLD_NEXT, "SDL_webOSSetExportedWindow");
    if (!hnd)
    {
        return SDL_FALSE;
    }
    return hnd(windowId, src, dst);
}

SDL_bool SDL_webOSExportedSetCropRegion(const char *windowId, SDL_Rect *org, SDL_Rect *src, SDL_Rect *dst)
{
    SDL_bool (*hnd)(const char *, SDL_Rect *, SDL_Rect *, SDL_Rect *) = dlsym(RTLD_NEXT, "SDL_webOSExportedSetCropRegion");
    if (!hnd)
    {
        return SDL_FALSE;
    }
    return hnd(windowId, org, src, dst);
}

SDL_bool SDL_webOSExportedSetProperty(const char *windowId, const char *name, const char *value)
{
    SDL_bool (*hnd)(const char *, const char *, const char *) = dlsym(RTLD_NEXT, "SDL_webOSExportedSetProperty");
    if (!hnd)
    {
        return SDL_FALSE;
    }
    return hnd(windowId, name, value);
}

void SDL_webOSDestroyExportedWindow(const char *windowId)
{
    void (*hnd)(const char *) = dlsym(RTLD_NEXT, "SDL_webOSDestroyExportedWindow");
    if (!hnd)
    {
        return;
    }
    hnd(windowId);
}