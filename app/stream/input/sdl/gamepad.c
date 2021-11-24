#include "stream/input/sdlinput.h"

#include "app.h"

#include <Limelight.h>
#include <stream/input/absinput.h>

#include "util/bus.h"
#include "util/user_event.h"

#define QUIT_BUTTONS (PLAY_FLAG | BACK_FLAG | LB_FLAG | RB_FLAG)
#define GAMEPAD_COMBO_VMOUSE1 (BACK_FLAG | RS_CLK_FLAG)
#define GAMEPAD_COMBO_VMOUSE2 (BACK_FLAG | LS_CLK_FLAG)

static bool quit_combo_pressed = false;
static bool vmouse_combo_pressed = false;

static void vmouse_set_vector(short x, short y);

static void vmouse_handle_button(Uint8 button, bool pressed);

static void release_buttons(PGAMEPAD_STATE gamepad);

static short calc_mouse_movement(short axis);

static struct {
    short x, y;
} vmouse_vector = {0, 0};

static SDL_TimerID vmouse_timer_id = 0;

static Uint32 vmouse_timer_callback(Uint32 interval, void *param);

static bool gamepad_combo_check(short buttons, short combo);

void sdlinput_handle_cbutton_event(SDL_ControllerButtonEvent *event) {
    short button = 0;
    PGAMEPAD_STATE gamepad = get_gamepad(event->which);
    bool vmouse_intercepted = false;
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
            if (absinput_virtual_mouse == ABSINPUT_VMOUSE_LEFT_STICK) {
                vmouse_intercepted = true;
            }
            button = LS_CLK_FLAG;
            break;
        case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
            if (absinput_virtual_mouse == ABSINPUT_VMOUSE_RIGHT_STICK) {
                vmouse_intercepted = true;
            }
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
    } else if (gamepad_combo_check(gamepad->buttons, GAMEPAD_COMBO_VMOUSE1)
               || gamepad_combo_check(gamepad->buttons, GAMEPAD_COMBO_VMOUSE2)) {
        vmouse_combo_pressed = true;
        return;
    } else if (vmouse_intercepted) {
        vmouse_handle_button(event->button, event->state);
        gamepad->buttons &= ~button;
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
                absinput_virtual_mouse = ABSINPUT_VMOUSE_OFF;
            } else {
                absinput_virtual_mouse = ABSINPUT_VMOUSE_RIGHT_STICK;
            }
            return;
        }
    }

    if (absinput_no_control)
        return;
    LiSendMultiControllerEvent(gamepad->id, activeGamepadMask, gamepad->buttons, gamepad->leftTrigger,
                               gamepad->rightTrigger,
                               gamepad->leftStickX, gamepad->leftStickY, gamepad->rightStickX, gamepad->rightStickY);
}

void sdlinput_handle_caxis_event(SDL_ControllerAxisEvent *event) {
    PGAMEPAD_STATE gamepad = get_gamepad(event->which);
    bool vmouse_intercepted = false;
    switch (event->axis) {
        case SDL_CONTROLLER_AXIS_LEFTX:
            gamepad->leftStickX = (short) -SDL_max(event->value, (short) -32767);
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
            if (absinput_virtual_mouse == ABSINPUT_VMOUSE_RIGHT_STICK) {
                vmouse_intercepted = true;
            }
            gamepad->rightStickX = (short) -SDL_max(event->value, (short) -32767);
            break;
        }
        case SDL_CONTROLLER_AXIS_RIGHTY: {
            if (absinput_virtual_mouse == ABSINPUT_VMOUSE_RIGHT_STICK) {
                vmouse_intercepted = true;
            }
            gamepad->rightStickY = (short) -SDL_max(event->value, (short) -32767);
            break;
        }
        case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
            gamepad->leftTrigger = (char) (event->value * 255UL / 32767);
            break;
        case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
            gamepad->rightTrigger = (char) (event->value * 255UL / 32767);
            break;
        default:
            return;
    }
    if (absinput_no_control)
        return;
    if (vmouse_intercepted) {
        if (absinput_virtual_mouse == ABSINPUT_VMOUSE_RIGHT_STICK) {
            vmouse_set_vector(gamepad->rightStickX, gamepad->rightStickY);
        } else if (absinput_virtual_mouse == ABSINPUT_VMOUSE_LEFT_STICK) {
            vmouse_set_vector(gamepad->leftStickX, gamepad->leftStickY);
        }
    } else {
        LiSendMultiControllerEvent(gamepad->id, activeGamepadMask, gamepad->buttons, gamepad->leftTrigger,
                                   gamepad->rightTrigger,
                                   gamepad->leftStickX, gamepad->leftStickY, gamepad->rightStickX,
                                   gamepad->rightStickY);
    }
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
    vmouse_vector.x = calc_mouse_movement((short) -x);
    vmouse_vector.y = calc_mouse_movement((short) -y);
    if (vmouse_vector.x || vmouse_vector.y) {
        if (!vmouse_timer_id) {
            vmouse_timer_id = SDL_AddTimer(0, vmouse_timer_callback, NULL);
        }
    } else if (vmouse_timer_id) {
        SDL_RemoveTimer(vmouse_timer_id);
        vmouse_timer_id = 0;
    }
}

static void vmouse_handle_button(Uint8 button, bool pressed) {
    switch (button) {
        case SDL_CONTROLLER_BUTTON_LEFTSTICK:
        case SDL_CONTROLLER_BUTTON_RIGHTSTICK: {
            LiSendMouseButtonEvent(pressed ? BUTTON_ACTION_PRESS : BUTTON_ACTION_RELEASE, BUTTON_LEFT);
            break;
        }
        default: {
            break;
        }
    }
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
                               gamepad->rightTrigger,
                               gamepad->leftStickX, gamepad->leftStickY, gamepad->rightStickX, gamepad->rightStickY);
}


static short calc_mouse_movement(short axis) {
    short abs_axis = (short) (axis > 0 ? axis : -axis);
    short threshold = 4096;
    if (abs_axis < threshold) return 0;
    return (short) (SDL_sqrt(abs_axis - threshold) * (axis > 0 ? 1 : -1));
}

static Uint32 vmouse_timer_callback(Uint32 interval, void *param) {
    if (absinput_virtual_mouse == ABSINPUT_VMOUSE_OFF) return 0;
    if (!vmouse_vector.x && !vmouse_vector.y) {
        return 0;
    }
    short speed = 16;
    double x = vmouse_vector.x / (32 - LV_CLAMP(0, speed, 16));
    double y = vmouse_vector.y / (32 - LV_CLAMP(0, speed, 16));
    double abs_x = LV_ABS(x), abs_y = LV_ABS(y);
    LiSendMouseMoveEvent((short) (abs_x > 1 ? x : x / abs_x), (short) (abs_y > 1 ? y : y / abs_y));
    return LV_CLAMP(5, 5 / LV_MAX(abs_x, abs_y), 20);
}

static bool gamepad_combo_check(short buttons, short combo) {
    return (buttons & combo) == combo;
}