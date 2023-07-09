#include "input_vmouse.h"
#include "absinput.h"

#include <SDL.h>

static short calc_mouse_movement(short axis);

static Uint32 vmouse_timer_callback(Uint32 interval, void *param);

void session_input_set_vmouse_active(session_input_vmouse_t *vmouse, bool active) {
    vmouse->state.active = active;
    if (!active) {
        vmouse_set_vector(vmouse, 0, 0);
        vmouse_set_trigger(vmouse, 0, 0);
    }
}

bool session_input_is_vmouse_active(session_input_vmouse_t *vmouse) {
    return vmouse->state.active;
}

void vmouse_set_vector(session_input_vmouse_t *vmouse, short x, short y) {
    vmouse->state.x = calc_mouse_movement(x);
    vmouse->state.y = calc_mouse_movement((short) -SDL_max(y, -32767));
    if (vmouse->state.x || vmouse->state.y) {
        if (!vmouse->timer_id) {
            vmouse->timer_id = SDL_AddTimer(0, vmouse_timer_callback, vmouse);
        }
    } else if (vmouse->timer_id) {
        SDL_RemoveTimer(vmouse->timer_id);
        vmouse->timer_id = 0;
    }
}

void vmouse_set_trigger(session_input_vmouse_t *vmouse, char l, char r) {
    const char trigger_threshold = 64;
    bool ldown = l > trigger_threshold, rdown = r > trigger_threshold;
    if (vmouse->state.l != ldown) {
        LiSendMouseButtonEvent(ldown ? BUTTON_ACTION_PRESS : BUTTON_ACTION_RELEASE, BUTTON_LEFT);
    }
    if (vmouse->state.r != rdown) {
        LiSendMouseButtonEvent(rdown ? BUTTON_ACTION_PRESS : BUTTON_ACTION_RELEASE, BUTTON_RIGHT);
    }
    vmouse->state.l = ldown;
    vmouse->state.r = rdown;
}

void vmouse_set_modifier(session_input_vmouse_t *vmouse, bool v) {
    vmouse->state.modifier = v;
}

static Uint32 vmouse_timer_callback(Uint32 interval, void *param) {
    (void) interval;
    session_input_vmouse_t *vmouse = param;
    if (!vmouse->state.active || (!vmouse->state.x && !vmouse->state.y)) {
        return 0;
    }
    short speed = 4;
    double speed_divider = 32 - SDL_max(0, SDL_min(speed, 16));
    double x = vmouse->state.x / speed_divider;
    double y = vmouse->state.y / speed_divider;
    double abs_x = SDL_fabs(x), abs_y = SDL_fabs(y);
    if (vmouse->state.modifier) {
        abs_y /= 20.0;
        LiSendHighResScrollEvent((short) (abs_y > 1 ? -y : -y / abs_y));
    } else {
        LiSendMouseMoveEvent((short) (abs_x > 1 ? x : x / abs_x), (short) (abs_y > 1 ? y : y / abs_y));
    }
    return SDL_max(5, SDL_min(5 / SDL_max(abs_x, abs_y), 20));
}

static short calc_mouse_movement(short axis) {
    short abs_axis = (short) (axis > 0 ? axis : -axis);
    short threshold = 4096;
    if (abs_axis < threshold) { return 0; }
    return (short) (SDL_sqrt(abs_axis - threshold) * (axis > 0 ? 1 : -1));
}