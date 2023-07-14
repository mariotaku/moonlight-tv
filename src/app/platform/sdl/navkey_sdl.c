#include "navkey_sdl.h"
#include "app.h"

#if TARGET_WEBOS

NAVKEY navkey_from_sdl_webos(const SDL_Event *ev);

#endif

static NAVKEY navkey_gamepad_map(SDL_JoystickID which, Uint8 button);

NAVKEY navkey_from_sdl(const SDL_Event *ev, bool *pressed) {
    switch (ev->type) {
        case SDL_KEYDOWN:
        case SDL_KEYUP: {
            *pressed = ev->key.state == SDL_PRESSED;
            switch (ev->key.keysym.sym) {
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
                default: {
#if TARGET_WEBOS
                    NAVKEY navkey;
                    if ((navkey = navkey_from_sdl_webos(ev)) != NAVKEY_UNKNOWN) {
                        return navkey;
                    }
#endif
                    return NAVKEY_UNKNOWN;
                }
            }
        }
        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP: {
            *pressed = ev->cbutton.state == SDL_PRESSED;
            return navkey_gamepad_map(ev->cbutton.which, ev->cbutton.button);
        }
        default:
            return NAVKEY_UNKNOWN;
    }
}

NAVKEY navkey_gamepad_map(SDL_JoystickID which, Uint8 button) {
    switch (button) {
        case SDL_CONTROLLER_BUTTON_DPAD_UP:
            return NAVKEY_UP;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
            return NAVKEY_DOWN;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
            return NAVKEY_LEFT;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
            return NAVKEY_RIGHT;
        case SDL_CONTROLLER_BUTTON_A:
            return app_configuration->swap_abxy ? NAVKEY_CANCEL : NAVKEY_CONFIRM;
        case SDL_CONTROLLER_BUTTON_B:
            return app_configuration->swap_abxy ? NAVKEY_CONFIRM : NAVKEY_CANCEL;
        case SDL_CONTROLLER_BUTTON_X:
            return app_configuration->swap_abxy ? NAVKEY_ALTERNATIVE : NAVKEY_NEGATIVE;
        case SDL_CONTROLLER_BUTTON_Y:
            return app_configuration->swap_abxy ? NAVKEY_NEGATIVE : NAVKEY_ALTERNATIVE;
        case SDL_CONTROLLER_BUTTON_BACK:
            return NAVKEY_MENU;
        case SDL_CONTROLLER_BUTTON_START:
            return NAVKEY_START;
        default:
            return NAVKEY_UNKNOWN;
    }
}