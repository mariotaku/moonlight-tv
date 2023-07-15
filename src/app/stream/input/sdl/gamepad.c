
#include "app.h"

#include <Limelight.h>
#include <SDL.h>

#include "stream/input/absinput.h"

#include "util/bus.h"
#include "util/user_event.h"
#include "input/input_gamepad.h"
#include "stream/session.h"
#include "stream/input/input_vmouse.h"

#define QUIT_BUTTONS (PLAY_FLAG | BACK_FLAG | LB_FLAG | RB_FLAG)
#define GAMEPAD_COMBO_VMOUSE (LB_FLAG | RS_CLK_FLAG)

static bool quit_combo_pressed = false;
static bool vmouse_combo_pressed = false;

static void release_buttons(stream_input_t *input, app_gamepad_state_t *gamepad);


static bool gamepad_combo_check(short buttons, short combo);

void stream_input_handle_cbutton(stream_input_t *input, const SDL_ControllerButtonEvent *event) {
    short button = 0;
    app_gamepad_state_t *gamepad = app_input_gamepad_get(input->input, event->which);
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
            vmouse_set_modifier(&input->vmouse, event->state == SDL_PRESSED);
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
            release_buttons(input, gamepad);
            bus_pushevent(USER_OPEN_OVERLAY, NULL, NULL);
            return;
        } else if (vmouse_combo_pressed) {
            vmouse_combo_pressed = false;
            release_buttons(input, gamepad);
            session_toggle_vmouse(input->session);
            return;
        }
    }

    if (input->view_only) {
        return;
    }
    LiSendMultiControllerEvent(gamepad->id, input->input->activeGamepadMask, gamepad->buttons, gamepad->leftTrigger,
                               gamepad->rightTrigger, gamepad->leftStickX, gamepad->leftStickY, gamepad->rightStickX,
                               gamepad->rightStickY);
}

void sdlinput_handle_caxis_event(stream_input_t *input, const SDL_ControllerAxisEvent *event) {
    app_gamepad_state_t *gamepad = app_input_gamepad_get(input->input, event->which);
    bool vmouse_intercepted = false;
    bool vmouse = session_input_is_vmouse_active(&input->vmouse);
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
    if (input->view_only) {
        return;
    }
    if (vmouse_intercepted) {
        vmouse_set_vector(&input->vmouse, gamepad->rightStickX, gamepad->rightStickY);
        vmouse_set_trigger(&input->vmouse, gamepad->leftTrigger, gamepad->rightTrigger);
    }
    if (vmouse) {
        LiSendMultiControllerEvent(gamepad->id, input->input->activeGamepadMask, gamepad->buttons, 0, 0,
                                   gamepad->leftStickX,
                                   gamepad->leftStickY, 0, 0);
    } else {
        LiSendMultiControllerEvent(gamepad->id, input->input->activeGamepadMask, gamepad->buttons, gamepad->leftTrigger,
                                   gamepad->rightTrigger, gamepad->leftStickX, gamepad->leftStickY,
                                   gamepad->rightStickX, gamepad->rightStickY);
    }
}


static void release_buttons(stream_input_t *input, app_gamepad_state_t *gamepad) {
    gamepad->buttons = 0;
    gamepad->leftTrigger = 0;
    gamepad->rightTrigger = 0;
    gamepad->leftStickX = 0;
    gamepad->leftStickY = 0;
    gamepad->rightStickX = 0;
    gamepad->rightStickY = 0;
    LiSendMultiControllerEvent(gamepad->id, input->input->activeGamepadMask, gamepad->buttons, gamepad->leftTrigger,
                               gamepad->rightTrigger, gamepad->leftStickX, gamepad->leftStickY, gamepad->rightStickX,
                               gamepad->rightStickY);
}


static bool gamepad_combo_check(short buttons, short combo) {
    return (buttons & combo) == combo;
}