#include "stream/input/session_input.h"

void stream_input_handle_touch(const stream_input_t *input, const SDL_TouchFingerEvent *event) {
    if (input->view_only) {
        return;
    }
    uint8_t type;
    switch (event->type) {
        case SDL_FINGERDOWN:
            type = LI_TOUCH_EVENT_DOWN;
            break;
        case SDL_FINGERUP:
            type = LI_TOUCH_EVENT_UP;
            break;
        case SDL_FINGERMOTION:
            type = LI_TOUCH_EVENT_MOVE;
            break;
        default:
            return;
    }
    LiSendTouchEvent(type, event->fingerId, event->x, event->y, event->pressure, 0, 0, 0);
}