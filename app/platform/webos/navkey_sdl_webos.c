#include "platform/sdl/navkey_sdl.h"
#include "platform/sdl/webos_keys.h"

NAVKEY navkey_from_sdl(SDL_Event ev)
{
    switch (ev.type)
    {
    case SDL_KEYDOWN:
    case SDL_KEYUP:
    {
        switch (ev.key.keysym.sym)
        {
        case 82 /* Keyboard/Remote Up */:
            return NAVKEY_UP;
        case 81 /* Keyboard/Remote Down */:
            return NAVKEY_DOWN;
        case 80 /* Keyboard/Remote Left */:
            return NAVKEY_LEFT;
        case 79 /* Keyboard/Remote Right */:
            return NAVKEY_RIGHT;
        case 40 /* Keyboard Enter */:
            return NAVKEY_ENTER;
        case 41 /* Keyboard ESC */:
        case SDLK_WEBOS_BACK:
            return NAVKEY_BACK;
        default:
            printf("KeyEvent, down: %d, key: %d\n", ev.key.state, ev.key.keysym.sym);
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
        case SDL_CONTROLLER_BUTTON_A:
            return NAVKEY_CONFIRM;
        case SDL_CONTROLLER_BUTTON_B:
            return NAVKEY_BACK;
        case SDL_CONTROLLER_BUTTON_BACK:
            return NAVKEY_MENU;
        case SDL_CONTROLLER_BUTTON_START:
            return NAVKEY_ENTER;
        default:
            break;
        }
    }
    default:
        return NAVKEY_UNKNOWN;
    }
}