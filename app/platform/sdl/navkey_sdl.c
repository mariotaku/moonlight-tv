#include "navkey_sdl.h"

NAVKEY navkey_from_sdl(SDL_Event ev)
{
    switch (ev.type)
    {
    case SDL_KEYDOWN:
    case SDL_KEYUP:
    {
        switch (ev.key.keysym.sym)
        {
        case SDLK_UP:
            return NAVKEY_UP;
        case SDLK_DOWN:
            return NAVKEY_DOWN;
        case SDLK_LEFT:
            return NAVKEY_LEFT;
        case SDLK_RIGHT:
            return NAVKEY_RIGHT;
        case SDLK_RETURN:
            return NAVKEY_ENTER;
        case SDLK_ESCAPE:
            return NAVKEY_BACK;
        default:
            return NAVKEY_UNKNOWN;
        }
    }
    case SDL_CONTROLLERBUTTONDOWN:
    case SDL_CONTROLLERBUTTONUP:
    {
        switch (ev.cbutton.button)
        {
        case SDL_CONTROLLER_BUTTON_DPAD_UP:
            return NAVKEY_UP;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
            return NAVKEY_DOWN;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
            return NAVKEY_LEFT;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
            return NAVKEY_RIGHT;
        default:
            break;
        }
    }
    default:
        return NAVKEY_UNKNOWN;
    }
}