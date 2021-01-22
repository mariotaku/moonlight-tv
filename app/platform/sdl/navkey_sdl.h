#pragma once

#include <SDL2/SDL.h>
#include "util/navkey.h"

NAVKEY navkey_from_sdl(SDL_Event ev);

static NAVKEY navkey_gamepad_map(Uint8 button)
{
    switch (button)
    {
    case SDL_CONTROLLER_BUTTON_DPAD_UP:
        return NAVKEY_UP;
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
        return NAVKEY_DOWN;
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
        return NAVKEY_LEFT;
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
        return NAVKEY_RIGHT;
    case SDL_CONTROLLER_BUTTON_A:
        return NAVKEY_CONFIRM;
    case SDL_CONTROLLER_BUTTON_B:
        return NAVKEY_BACK;
    case SDL_CONTROLLER_BUTTON_X:
        return NAVKEY_CLOSE;
    case SDL_CONTROLLER_BUTTON_BACK:
        return NAVKEY_MENU;
    case SDL_CONTROLLER_BUTTON_START:
        return NAVKEY_ENTER;
    default:
        return NAVKEY_UNKNOWN;
    }
}