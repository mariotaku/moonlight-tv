#include <SDL_webOS.h>

#include <dlfcn.h>

#define SDL_BACKPORT __attribute__((unused))
#define SDL_CONTROLLER_PLATFORM_FIELD   "platform:"

static int noop() {
    return 0;
}

SDL_BACKPORT SDL_bool SDL_webOSCursorVisibility(SDL_bool visible) {
    static SDL_bool (*fn)(SDL_bool) = (void *) noop;
    if (fn == (void *) noop) {
        fn = dlsym(RTLD_NEXT, "SDL_webOSCursorVisibility");
    }
    if (fn == NULL) {
        return SDL_FALSE;
    }
    return fn(visible);
}

SDL_BACKPORT SDL_bool SDL_webOSGetPanelResolution(int *width, int *height) {
    static SDL_bool (*fn)(int *, int *) =(void *) noop;
    if (fn == (void *) noop) {
        fn = dlsym(RTLD_NEXT, "SDL_webOSGetPanelResolution");
    }
    if (fn == NULL) {
        return SDL_FALSE;
    }
    return fn(width, height);
}

SDL_BACKPORT SDL_bool SDL_webOSGetRefreshRate(int *rate) {
    static SDL_bool (*fn)(int *) =(void *) noop;
    if (fn == (void *) noop) {
        fn = dlsym(RTLD_NEXT, "SDL_webOSGetRefreshRate");
    }
    if (fn == NULL) {
        return SDL_FALSE;
    }
    return fn(rate);
}