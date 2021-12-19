#include "stream/input/absinput.h"

#include "app.h"

#include "stream/session.h"

#include <Limelight.h>
#include <SDL.h>

#if TARGET_WEBOS

bool webos_magic_remote_active();

#endif

void sdlinput_handle_mbutton_event(SDL_MouseButtonEvent *event) {
    if (absinput_no_control)
        return;
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

    LiSendMouseButtonEvent(event->type == SDL_MOUSEBUTTONDOWN ? BUTTON_ACTION_PRESS : BUTTON_ACTION_RELEASE,
                           button);
}

void sdlinput_handle_mwheel_event(SDL_MouseWheelEvent *event) {
    if (absinput_no_control) return;
    LiSendScrollEvent((signed char) event->y);
}

void sdlinput_handle_mmotion_event(SDL_MouseMotionEvent *event) {
    if (absinput_no_control) return;
#if TARGET_WEBOS
    if (webos_magic_remote_active()) return;
#endif
    if (app_get_mouse_relative()) {
        LiSendMouseMoveEvent((short) event->xrel, (short) event->yrel);
    } else {
        LiSendMousePositionEvent((short) event->x, (short) event->y, streaming_display_width, streaming_display_height);
    }
}
