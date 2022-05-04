#include "app.h"

#include "ui/root.h"

#include "stream/input/absinput.h"
#include "stream/input/sdl/vk.h"
#include "stream/session.h"

#include <Limelight.h>
#include <SDL.h>

#include "util/bus.h"
#include "util/user_event.h"

bool webos_intercept_remote_keys(SDL_KeyboardEvent *event, short *keyCode) {
    switch ((unsigned int) event->keysym.scancode) {
        case SDL_WEBOS_SCANCODE_EXIT: {
            if (event->state == SDL_PRESSED) {
                bus_pushevent(USER_OPEN_OVERLAY, NULL, NULL);
            }
            return true;
        }
        case SDL_WEBOS_SCANCODE_BACK:
            *keyCode = VK_ESCAPE /* SDL_SCANCODE_ESCAPE */;
            return false;
        case SDL_WEBOS_SCANCODE_CH_UP:
            *keyCode = VK_PRIOR /* SDL_SCANCODE_PAGEUP */;
            return false;
        case SDL_WEBOS_SCANCODE_CH_DOWN:
            *keyCode = VK_NEXT /* SDL_SCANCODE_PAGEDOWN */;
            return false;
        case SDL_WEBOS_SCANCODE_YELLOW:
            if (absinput_no_control) return true;
            if (ui_input_mode & UI_INPUT_MODE_POINTER_FLAG) {
                LiSendMouseButtonEvent(event->type == SDL_KEYDOWN ? BUTTON_ACTION_PRESS : BUTTON_ACTION_RELEASE,
                                       BUTTON_RIGHT);
            } else {
                *keyCode = VK_MENU;
            }
        default:
            return false;
    }
}