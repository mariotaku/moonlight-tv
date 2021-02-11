#include "navkey_sdl.h"

#if OS_WEBOS
NAVKEY navkey_from_sdl_webos(SDL_Event ev);
#endif

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
            return NAVKEY_CONFIRM;
        case SDLK_ESCAPE:
            return NAVKEY_CANCEL;
        case SDLK_BACKSPACE:
            return NAVKEY_NEGATIVE;
        case SDLK_MENU:
        case SDLK_APPLICATION:
            return NAVKEY_MENU;
        default:
        {
#if OS_WEBOS
            NAVKEY navkey;
            if (navkey = navkey_from_sdl_webos(ev))
            {
                return navkey;
            }
#endif
            return NAVKEY_UNKNOWN;
        }
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