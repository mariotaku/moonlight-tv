#include "mmal_common.h"

#include <SDL.h>

bool mmalvid_set_region(bool fullscreen, int x, int y, int w, int h);

static void presenter_enter_fullscreen() {
    mmalvid_set_region(true, 0, 0, 0, 0);
    SDL_ShowCursor(SDL_FALSE);
}

static void presenter_enter_overlay(int x, int y, int w, int h) {
    mmalvid_set_region(false, x, y, w, h);
    SDL_ShowCursor(SDL_TRUE);
}

MODULE_API VIDEO_PRESENTER_CALLBACKS presenter_callbacks_mmal = {
        .enterFullScreen = presenter_enter_fullscreen,
        .enterOverlay = presenter_enter_overlay,
};