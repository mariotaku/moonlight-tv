#include "stream/input/absinput.h"

#include "app.h"

#include "stream/session.h"
#include "stream/session/priv.h"

#include <Limelight.h>
#include <SDL.h>

void sdlinput_handle_mbutton_event(const SDL_MouseButtonEvent *event) {
    int button = 0;
    switch (event->button) {
        case SDL_BUTTON_LEFT:
            button = BUTTON_LEFT;
            break;
        case SDL_BUTTON_MIDDLE:
            button = BUTTON_MIDDLE;
            break;
        case SDL_BUTTON_RIGHT:
            button = BUTTON_RIGHT;
            break;
        case SDL_BUTTON_X1:
            button = BUTTON_X1;
            break;
        case SDL_BUTTON_X2:
            button = BUTTON_X2;
            break;
        default:
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                        "Unhandled button event: %d",
                        event->button);
            return;
    }
    LiSendMouseButtonEvent(event->state == SDL_PRESSED ? BUTTON_ACTION_PRESS : BUTTON_ACTION_RELEASE,
                           button);
}

void sdlinput_handle_mwheel_event(const SDL_MouseWheelEvent *event) {
    LiSendScrollEvent((signed char) event->y);
}

void session_handle_mmotion_event(stream_input_t *input, const SDL_MouseMotionEvent *event) {
    if (absinput_no_control || absinput_no_sdl_mouse) { return; }
    if (app_get_mouse_relative()) {
        LiSendMouseMoveEvent((short) event->xrel, (short) event->yrel);
    } else {
        LiSendMousePositionEvent((short) event->x, (short) event->y, input->session->display_width,
                                 input->session->display_height);
    }
}
