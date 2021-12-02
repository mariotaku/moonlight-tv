#include "stream/input/sdlinput.h"

#include "app.h"

#include <Limelight.h>
#include <SDL.h>

#include "stream/input/absinput.h"

#include "util/bus.h"
#include "util/user_event.h"

#define QUIT_BUTTONS (PLAY_FLAG | BACK_FLAG | LB_FLAG | RB_FLAG)
#define GAMEPAD_COMBO_VMOUSE (LB_FLAG | RS_CLK_FLAG)

static bool quit_combo_pressed = false;
static bool vmouse_combo_pressed = false;

static void vmouse_set_vector(short x, short y);

static void vmouse_set_trigger(char l, char r);

static void release_buttons(PGAMEPAD_STATE gamepad);

static short calc_mouse_movement(short axis);

static struct {
    short x, y;
    bool l, r;
    bool modifier;
} vmouse_state = {0, 0, 0, 0};

static SDL_TimerID vmouse_timer_id = 0;

static Uint32 vmouse_timer_callback(Uint32 interval, void *param);

static bool gamepad_combo_check(short buttons, short combo);

void sdlinput_handle_cbutton_event(SDL_ControllerButtonEvent *event) {
    short button = 0;
    PGAMEPAD_STATE gamepad = get_gamepad(event->which);
    switch (event->button) {
        case SDL_CONTROLLER_BUTTON_A:
            button = app_configuration->swap_abxy ? B_FLAG : A_FLAG;
            break;
        case SDL_CONTROLLER_BUTTON_B:
            button = app_configuration->swap_abxy ? A_FLAG : B_FLAG;
            break;
        case SDL_CONTROLLER_BUTTON_Y:
            button = app_configuration->swap_abxy ? X_FLAG : Y_FLAG;
            break;
        case SDL_CONTROLLER_BUTTON_X:
            button = app_configuration->swap_abxy ? Y_FLAG : X_FLAG;
            break;
        case SDL_CONTROLLER_BUTTON_DPAD_UP:
            button = UP_FLAG;
            break;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
            button = DOWN_FLAG;
            break;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
            button = RIGHT_FLAG;
            break;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
            button = LEFT_FLAG;
            break;
        case SDL_CONTROLLER_BUTTON_BACK:
            button = BACK_FLAG;
            break;
        case SDL_CONTROLLER_BUTTON_START:
            button = PLAY_FLAG;
            break;
        case SDL_CONTROLLER_BUTTON_GUIDE:
            button = SPECIAL_FLAG;
            break;
        case SDL_CONTROLLER_BUTTON_LEFTSTICK:
            button = LS_CLK_FLAG;
            vmouse_state.modifier = event->state == SDL_PRESSED;
            break;
        case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
            button = RS_CLK_FLAG;
            break;
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
            button = LB_FLAG;
            break;
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
            button = RB_FLAG;
            break;
        default:
            return;
    }
    if (event->type == SDL_CONTROLLERBUTTONDOWN) {
        gamepad->buttons |= button;
    } else {
        gamepad->buttons &= ~button;
    }

    if (gamepad_combo_check(gamepad->buttons, QUIT_BUTTONS)) {
        quit_combo_pressed = true;
        return;
    } else if (gamepad_combo_check(gamepad->buttons, GAMEPAD_COMBO_VMOUSE)) {
        vmouse_combo_pressed = true;
        return;
    }
    if (gamepad->buttons == 0) {
        if (quit_combo_pressed) {
            quit_combo_pressed = false;
            release_buttons(gamepad);
            bus_pushevent(USER_OPEN_OVERLAY, NULL, NULL);
            return;
        } else if (vmouse_combo_pressed) {
            vmouse_combo_pressed = false;
            release_buttons(gamepad);
            if (absinput_virtual_mouse) {
                vmouse_set_vector(0, 0);
                vmouse_set_trigger(0, 0);
                absinput_virtual_mouse = false;
            } else {
                absinput_virtual_mouse = true;
            }
            return;
        }
    }

    if (absinput_no_control)
        return;
    LiSendMultiControllerEvent(gamepad->id, activeGamepadMask, gamepad->buttons, gamepad->leftTrigger,
                               gamepad->rightTrigger, gamepad->leftStickX, gamepad->leftStickY, gamepad->rightStickX,
                               gamepad->rightStickY);
}

void sdlinput_handle_caxis_event(SDL_ControllerAxisEvent *event) {
    PGAMEPAD_STATE gamepad = get_gamepad(event->which);
    bool vmouse_intercepted = false;
    bool vmouse = absinput_virtual_mouse;
    switch (event->axis) {
        case SDL_CONTROLLER_AXIS_LEFTX:
            gamepad->leftStickX = SDL_max(event->value, -32767);
            break;
        case SDL_CONTROLLER_AXIS_LEFTY:
            // Signed values have one more negative value than
            // positive value, so inverting the sign on -32768
            // could actually cause the value to overflow and
            // wrap around to be negative again. Avoid that by
            // capping the value at 32767.
            gamepad->leftStickY = (short) -SDL_max(event->value, (short) -32767);
            break;
        case SDL_CONTROLLER_AXIS_RIGHTX: {
            if (vmouse) {
                vmouse_intercepted = true;
            }
            gamepad->rightStickX = SDL_max(event->value, -32767);
            break;
        }
        case SDL_CONTROLLER_AXIS_RIGHTY: {
            if (vmouse) {
                vmouse_intercepted = true;
            }
            gamepad->rightStickY = (short) -SDL_max(event->value, (short) -32767);
            break;
        }
        case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
            if (vmouse) {
                vmouse_intercepted = true;
            }
            gamepad->leftTrigger = (char) (event->value * 255UL / 32767);
            break;
        case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
            if (vmouse) {
                vmouse_intercepted = true;
            }
            gamepad->rightTrigger = (char) (event->value * 255UL / 32767);
            break;
        default:
            return;
    }
    if (absinput_no_control)
        return;
    if (vmouse_intercepted) {
        vmouse_set_vector(gamepad->rightStickX, gamepad->rightStickY);
        vmouse_set_trigger(gamepad->leftTrigger, gamepad->rightTrigger);
    }
    LiSendMultiControllerEvent(gamepad->id, activeGamepadMask, gamepad->buttons,
                               vmouse ? 0 : gamepad->leftTrigger,
                               vmouse ? 0 : gamepad->rightTrigger,
                               gamepad->leftStickX, gamepad->leftStickY,
                               (short) (vmouse ? 0 : gamepad->rightStickX),
                               (short) (vmouse ? 0 : gamepad->rightStickY));
}

PGAMEPAD_STATE get_gamepad(SDL_JoystickID sdl_id) {
    for (short i = 0; i < 4; i++) {
        if (!gamepads[i].initialized) {
            gamepads[i].sdl_id = sdl_id;
            gamepads[i].id = i;
            gamepads[i].initialized = true;
            activeGamepadMask |= (1 << i);
            return &gamepads[i];
        } else if (gamepads[i].sdl_id == sdl_id)
            return &gamepads[i];
    }
    return &gamepads[0];
}

static void vmouse_set_vector(short x, short y) {
    vmouse_state.x = calc_mouse_movement(x);
    vmouse_state.y = calc_mouse_movement((short) -SDL_max(y, -32767));
    if (vmouse_state.x || vmouse_state.y) {
        if (!vmouse_timer_id) {
            vmouse_timer_id = SDL_AddTimer(0, vmouse_timer_callback, NULL);
        }
    } else if (vmouse_timer_id) {
        SDL_RemoveTimer(vmouse_timer_id);
        vmouse_timer_id = 0;
    }
}

static void vmouse_set_trigger(char l, char r) {
    const char trigger_threshold = 64;
    bool ldown = l > trigger_threshold, rdown = r > trigger_threshold;
    if (vmouse_state.l != ldown) {
        LiSendMouseButtonEvent(ldown ? BUTTON_ACTION_PRESS : BUTTON_ACTION_RELEASE, BUTTON_LEFT);
    }
    if (vmouse_state.r != rdown) {
        LiSendMouseButtonEvent(rdown ? BUTTON_ACTION_PRESS : BUTTON_ACTION_RELEASE, BUTTON_RIGHT);
    }
    vmouse_state.l = ldown;
    vmouse_state.r = rdown;
}

static void release_buttons(PGAMEPAD_STATE gamepad) {
    gamepad->buttons = 0;
    gamepad->leftTrigger = 0;
    gamepad->rightTrigger = 0;
    gamepad->leftStickX = 0;
    gamepad->leftStickY = 0;
    gamepad->rightStickX = 0;
    gamepad->rightStickY = 0;
    LiSendMultiControllerEvent(gamepad->id, activeGamepadMask, gamepad->buttons, gamepad->leftTrigger,
                               gamepad->rightTrigger, gamepad->leftStickX, gamepad->leftStickY, gamepad->rightStickX,
                               gamepad->rightStickY);
}


static short calc_mouse_movement(short axis) {
    short abs_axis = (short) (axis > 0 ? axis : -axis);
    short threshold = 4096;
    if (abs_axis < threshold) return 0;
    return (short) (SDL_sqrt(abs_axis - threshold) * (axis > 0 ? 1 : -1));
}

static Uint32 vmouse_timer_callback(Uint32 interval, void *param) {
    if (!absinput_virtual_mouse) return 0;
    if (!vmouse_state.x && !vmouse_state.y) {
        return 0;
    }
    short speed = 4;
    double speed_divider = 32 - SDL_max(0, SDL_min(speed, 16));
    double x = vmouse_state.x / speed_divider;
    double y = vmouse_state.y / speed_divider;
    double abs_x = SDL_fabs(x), abs_y = SDL_fabs(y);
    if (vmouse_state.modifier) {
        abs_y /= 20.0;
        LiSendHighResScrollEvent((short) (abs_y > 1 ? -y : -y / abs_y));
    } else {
        LiSendMouseMoveEvent((short) (abs_x > 1 ? x : x / abs_x), (short) (abs_y > 1 ? y : y / abs_y));
    }
    return SDL_max(5, SDL_min(5 / SDL_max(abs_x, abs_y), 20));
}

static bool gamepad_combo_check(short buttons, short combo) {
    return (buttons & combo) == combo;
}