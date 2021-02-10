#include "platform/sdl/navkey_sdl.h"
#include "platform/sdl/webos_keys.h"

#include <stdio.h>

NAVKEY navkey_from_sdl(SDL_Event ev)
{
    switch (ev.type)
    {
    case SDL_KEYDOWN:
    case SDL_KEYUP:
    {
        switch (ev.key.keysym.sym)
        {
        case SDLK_UP /* Keyboard/Remote Up */:
            return NAVKEY_UP;
        case SDLK_DOWN /* Keyboard/Remote Down */:
            return NAVKEY_DOWN;
        case SDLK_LEFT /* Keyboard/Remote Left */:
            return NAVKEY_LEFT;
        case SDLK_RIGHT /* Keyboard/Remote Right */:
            return NAVKEY_RIGHT;
        case SDLK_RETURN /* Keyboard Enter */:
            return NAVKEY_START;
        case SDLK_ESCAPE /* Keyboard ESC */:
            return NAVKEY_NEGATIVE;
        default:
            switch (ev.key.keysym.scancode)
            {
            case SDL_WEBOS_SCANCODE_RED:
                return NAVKEY_NEGATIVE;
            case SDL_WEBOS_SCANCODE_YELLOW:
                return NAVKEY_MENU;
            case SDL_WEBOS_SCANCODE_BACK:
                return NAVKEY_CANCEL;
            default:
                break;
            }
            return NAVKEY_UNKNOWN;
        }
    }
    case SDL_CONTROLLERBUTTONDOWN:
    case SDL_CONTROLLERBUTTONUP:
    {
        return navkey_gamepad_map(ev.cbutton.button);
    }
    default:
        return NAVKEY_UNKNOWN;
    }
}