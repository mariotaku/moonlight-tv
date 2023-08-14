#include "platform/sdl/navkey_sdl.h"

NAVKEY navkey_from_sdl_webos(const SDL_Event *ev)
{
    switch (ev->type)
    {
    case SDL_KEYDOWN:
    case SDL_KEYUP:
    {
        switch ((int) ev->key.keysym.scancode)
        {
        case SDL_SCANCODE_WEBOS_RED:
            return NAVKEY_NEGATIVE;
        case SDL_SCANCODE_WEBOS_YELLOW:
            return NAVKEY_MENU;
        case SDL_SCANCODE_WEBOS_BLUE:
            return NAVKEY_ALTERNATIVE;
        case SDL_SCANCODE_WEBOS_BACK:
            return NAVKEY_CANCEL;
        default:
            break;
        }
    }
    default:
        return NAVKEY_UNKNOWN;
    }
}