#include "app.h"

#include <Limelight.h>

#include "stream/input/session_input.h"

#include "util/bus.h"
#include "util/user_event.h"
#include "input/input_gamepad.h"
#include "stream/session.h"
#include "stream/input/session_virt_mouse.h"
#include "logging.h"

#define QUIT_BUTTONS (PLAY_FLAG | BACK_FLAG | LB_FLAG | RB_FLAG)
#define GAMEPAD_COMBO_VMOUSE (LB_FLAG | RS_CLK_FLAG)

static bool quit_combo_pressed = false;
static bool vmouse_combo_pressed = false;

static void release_buttons(stream_input_t *input, app_gamepad_state_t *gamepad);

static bool gamepad_combo_check(int buttons, short combo);


void stream_input_handle_cbutton(stream_input_t *input, const SDL_ControllerButtonEvent *event) {
    int button = 0;
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
        case SDL_CONTROLLER_BUTTON_TOUCHPAD:
            button = TOUCHPAD_FLAG;
            break;
        case SDL_CONTROLLER_BUTTON_MISC1:
            button = MISC_FLAG;
            break;
        case SDL_CONTROLLER_BUTTON_PADDLE1:
            button = PADDLE1_FLAG;
            break;
        case SDL_CONTROLLER_BUTTON_PADDLE2:
            button = PADDLE2_FLAG;
            break;
        case SDL_CONTROLLER_BUTTON_PADDLE3:
            button = PADDLE3_FLAG;
            break;
        case SDL_CONTROLLER_BUTTON_PADDLE4:
            button = PADDLE4_FLAG;
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

void stream_input_handle_caxis(stream_input_t *input, const SDL_ControllerAxisEvent *event) {
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
                                   gamepad->leftStickX, gamepad->leftStickY, 0, 0);
    } else {
        LiSendMultiControllerEvent(gamepad->id, input->input->activeGamepadMask, gamepad->buttons, gamepad->leftTrigger,
                                   gamepad->rightTrigger, gamepad->leftStickX, gamepad->leftStickY,
                                   gamepad->rightStickX, gamepad->rightStickY);
    }
}

void stream_input_handle_csensor(stream_input_t *input, const SDL_ControllerSensorEvent *event) {
    if (input->view_only) {
        return;
    }
    app_gamepad_state_t *gamepad = app_input_gamepad_get(input->input, event->which);
    uint8_t event_type;
    switch (event->sensor) {
        case SDL_SENSOR_ACCEL: {
            event_type = LI_MOTION_TYPE_ACCEL;
            break;
        }
        case SDL_SENSOR_GYRO: {
            event_type = LI_MOTION_TYPE_GYRO;
            break;
        }
        default: {
            return;
        }
    }
    LiSendControllerMotionEvent(gamepad->id, event_type, event->data[0], event->data[1], event->data[2]);
}

void stream_input_handle_ctouchpad(stream_input_t *input, const SDL_ControllerTouchpadEvent *event) {
    if (input->view_only) {
        return;
    }
    if (event->touchpad != 0) {
        return;
    }
    app_gamepad_state_t *gamepad = app_input_gamepad_get(input->input, event->which);
    uint8_t event_type;
    switch (event->type) {
        case SDL_CONTROLLERTOUCHPADUP: {
            event_type = LI_TOUCH_EVENT_UP;
            break;
        }
        case SDL_CONTROLLERTOUCHPADDOWN: {
            event_type = LI_TOUCH_EVENT_DOWN;
            break;
        }
        case SDL_CONTROLLERTOUCHPADMOTION: {
            event_type = LI_TOUCH_EVENT_MOVE;
            break;
        }
        default: {
            return;
        }
    }
    LiSendControllerTouchEvent(gamepad->id, event_type, event->finger, event->x, event->y, event->pressure);
}

void stream_input_handle_cdevice(stream_input_t *input, const SDL_ControllerDeviceEvent *event) {
    app_gamepad_state_t *gamepad = app_input_gamepad_get(input->input, event->which);
    if (input->view_only) {
        return;
    }
    stream_input_send_gamepad_arrive(input, gamepad);
}

void stream_input_send_gamepad_arrive(const stream_input_t *input, app_gamepad_state_t *gamepad) {
    uint8_t type = LI_CTYPE_XBOX;
    uint16_t capabilities = LI_CCAP_ANALOG_TRIGGERS;
    commons_log_info("Input", "Controller %d arrived. Name: %s", gamepad->id,
                     SDL_GameControllerName(gamepad->controller));
    switch (SDL_GameControllerGetType(gamepad->controller)) {
#if SDL_VERSION_ATLEAST(2, 24, 0)
        case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_PAIR:
        case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_LEFT:
        case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT:
#endif
        case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO: {
            type = LI_CTYPE_NINTENDO;
            capabilities &= ~LI_CCAP_ANALOG_TRIGGERS;
            break;
        }
        case SDL_CONTROLLER_TYPE_PS3: {
            type = LI_CTYPE_PS;
            break;
        }
        case SDL_CONTROLLER_TYPE_PS4:
        case SDL_CONTROLLER_TYPE_PS5: {
            type = LI_CTYPE_PS;
            capabilities |= LI_CCAP_TOUCHPAD;
            commons_log_info("Input", "  controller capability: touchpad");
            break;
        }
        default: {
            break;
        }
    }
#if SDL_VERSION_ATLEAST(2, 0, 18)
    if (SDL_GameControllerHasRumble(gamepad->controller)) {
        capabilities |= LI_CCAP_RUMBLE;
        commons_log_info("Input", "  controller capability: rumble");
    }
    if (SDL_GameControllerHasRumbleTriggers(gamepad->controller)) {
        capabilities |= LI_CCAP_TRIGGER_RUMBLE;
        commons_log_info("Input", "  controller capability: trigger rumble");
    }
#else
    capabilities |= LI_CCAP_RUMBLE;
#if SDL_VERSION_ATLEAST(2, 0, 14)
        capabilities |= LI_CCAP_TRIGGER_RUMBLE;
#endif
#endif
#if SDL_VERSION_ATLEAST(2, 0, 14)
    if (SDL_GameControllerHasSensor(gamepad->controller, SDL_SENSOR_ACCEL)) {
        capabilities |= LI_CCAP_ACCEL;
        commons_log_info("Input", "  controller capability: accelerometer");
    }
    if (SDL_GameControllerHasSensor(gamepad->controller, SDL_SENSOR_GYRO)) {
        capabilities |= LI_CCAP_GYRO;
        commons_log_info("Input", "  controller capability: gyroscope");
    }
    if (SDL_GameControllerHasLED(gamepad->controller)) {
        capabilities |= LI_CCAP_RGB_LED;
        commons_log_info("Input", "  controller capability: RGB LED");
    }
#endif
    LiSendControllerArrivalEvent(gamepad->id, input->input->activeGamepadMask, type, 0xFFFFFFFF, capabilities);
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


static bool gamepad_combo_check(int buttons, short combo) {
    return (buttons & combo) == combo;
}