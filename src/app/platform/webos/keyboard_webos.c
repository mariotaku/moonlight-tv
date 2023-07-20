#include "app.h"

#include "ui/root.h"

#include "stream/input/session_input.h"
#include "stream/input/vk.h"
#include "stream/session.h"
#include "stream/session/priv.h"

#include <Limelight.h>
#include <SDL.h>

#include "util/bus.h"
#include "util/user_event.h"

#define TV_REMOTE_TOGGLE_SOFT_INPUT 0

bool webos_intercept_remote_keys(stream_input_t *input, const SDL_KeyboardEvent *event, short *keyCode) {
    session_t *session = input->session;
    app_t *app = session->app;
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
            if (input->view_only) {
                return true;
            }
            if (app_ui_get_input_mode(&app->ui.input) & UI_INPUT_MODE_POINTER_FLAG) {
                LiSendMouseButtonEvent(event->type == SDL_KEYDOWN ? BUTTON_ACTION_PRESS : BUTTON_ACTION_RELEASE,
                                       BUTTON_RIGHT);
                return true;
            } else {
                *keyCode = VK_MENU;
                return false;
            }
#if TV_REMOTE_TOGGLE_SOFT_INPUT
            case SDL_WEBOS_SCANCODE_BLUE:
                if (absinput_no_control) return true;
                if (!app_text_input_active()) {
                    app_start_text_input(0, 0, ui_display_width, ui_display_height);
                } else {
                    app_stop_text_input();
                }
                return true;
            case SDL_WEBOS_SCANCODE_GREEN:
                if (absinput_no_control) return true;
                session_toggle_vmouse(session);
                return true;
#endif
        default:
            return false;
    }
}