#include "app.h"

#include <Limelight.h>

#include "app_settings.h"
#include "stream/input/session_input.h"

#include "util/bus.h"
#include "util/user_event.h"
#include "input/input_gamepad.h"
#include "stream/session.h"
#include "stream/input/session_virt_mouse.h"
#include "logging.h"

#define QUIT_BUTTONS (PLAY_FLAG | BACK_FLAG | LB_FLAG | RB_FLAG)
#define GAMEPAD_COMBO_VMOUSE (LB_FLAG | RS_CLK_FLAG)

#define TOUCHPAD_SECONDARY_CORNER 0.75f
#define TOUCHPAD_TAP_THRESHOLD_MS 300u
#define TOUCHPAD_SINGLE_TAP_SLOP_SQ (0.015f * 0.015f)
#define TOUCHPAD_TWO_FINGER_TAP_CHORD_MS 160u
#define TOUCHPAD_TWO_FINGER_TAP_SLOP_SQ (0.035f * 0.035f)
#define TOUCHPAD_TRACKED_FINGERS 2

enum {
    TOUCHPAD_TAP_NONE = 0,
    TOUCHPAD_TAP_SINGLE_PENDING,
    TOUCHPAD_TAP_SINGLE_HOLD,
    TOUCHPAD_TAP_TWO_PENDING,
};

enum {
    TOUCHPAD_MOTION_NONE = 0,
    TOUCHPAD_MOTION_SCROLL,
    TOUCHPAD_MOTION_POINTER_0,
    TOUCHPAD_MOTION_POINTER_1,
};

typedef struct touchpad_finger_t {
    float x, y;
    float down_x, down_y;
    uint32_t down_timestamp;
} touchpad_finger_t;

struct session_input_touchpad_t {
    touchpad_finger_t fingers[TOUCHPAD_TRACKED_FINGERS];
    uint8_t active_fingers;
    float motion_remainder_x, motion_remainder_y;
    int8_t physical_mouse_button;
    uint8_t motion_state;
    uint8_t tap_state;
};

static bool quit_combo_pressed = false;
static bool vmouse_combo_pressed = false;

static void release_buttons(stream_input_t *input, app_gamepad_state_t *gamepad);

static bool gamepad_combo_check(int buttons, short combo);

static bool sensor_state_needs_update(const app_gamepad_sensor_state_t *state, uint32_t timestamp,
                                      const float data[3]);

static bool vmouse_intercepted(stream_input_t *input, const app_gamepad_state_t *gamepad);

static bool filter_deadzone_2axis(stream_input_t *input, short *x, short *y);

static session_input_touchpad_t *touchpad_state(
        stream_input_t *input, const app_gamepad_state_t *gamepad);

static void touchpad_reset_state(session_input_touchpad_t *state);

static bool controller_has_touchpad(const app_gamepad_state_t *gamepad);

static bool touchpad_multitouch_enabled(const stream_input_t *input, const app_gamepad_state_t *gamepad);

static float touchpad_aspect(const app_gamepad_state_t *gamepad);

static int touchpad_primary_finger(const session_input_touchpad_t *state);

static bool touchpad_lower_right_press(const session_input_touchpad_t *state);

static bool touchpad_finger_exceeded_tap_slop(
        const touchpad_finger_t *finger, float threshold_squared);

static void touchpad_end_tap(session_input_touchpad_t *state);

static void touchpad_send_mouse_click(int mouse_button);

static short touchpad_take_delta(float *remainder);

void stream_input_touchpad_mouse_init(stream_input_t *input) {
    if (input->touchpads != NULL ||
        input->touchpad_mode != TOUCHPAD_MODE_MOUSE) {
        return;
    }

    short count = app_input_get_max_gamepads(input->input);
    if (count <= 0) {
        return;
    }

    input->touchpads = SDL_calloc((size_t) count, sizeof(*input->touchpads));
    if (input->touchpads != NULL) {
        input->touchpad_count = count;
    }
}

void stream_input_touchpad_mouse_deinit(stream_input_t *input) {
    if (input->touchpads != NULL) {
        for (short i = 0; i < input->touchpad_count; ++i) {
            touchpad_reset_state(&input->touchpads[i]);
        }
        SDL_free(input->touchpads);
        input->touchpads = NULL;
    }
    input->touchpad_count = 0;
}

void stream_input_handle_cbutton(stream_input_t *input, const SDL_ControllerButtonEvent *event) {
    app_gamepad_state_t *gamepad = app_input_gamepad_state_by_instance_id(input->input, event->which);
    if (gamepad == NULL) {
        return;
    }
    int button = 0;
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
            if (input->touchpad_mode == TOUCHPAD_MODE_MOUSE) {
                if (input->view_only) {
                    return;
                }

                session_input_touchpad_t *state = touchpad_state(input, gamepad);
                int mouse_button = BUTTON_LEFT;
                if (event->type == SDL_CONTROLLERBUTTONDOWN) {
                    if (state != NULL) {
                        // A physical click always wins over a tap gesture or drag.
                        touchpad_end_tap(state);
                        if (touchpad_lower_right_press(state)) {
                            mouse_button = BUTTON_RIGHT;
                        }
                        state->physical_mouse_button = (int8_t) mouse_button;
                    }
                } else if (state != NULL) {
                    mouse_button = state->physical_mouse_button > 0 ? state->physical_mouse_button : BUTTON_LEFT;
                    state->physical_mouse_button = 0;
                }

                LiSendMouseButtonEvent(event->type == SDL_CONTROLLERBUTTONDOWN ?
                                       BUTTON_ACTION_PRESS : BUTTON_ACTION_RELEASE, mouse_button);
                return;
            }
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
    LiSendMultiControllerEvent(gamepad->gs_id, input->input->activeGamepadMask, gamepad->buttons, gamepad->leftTrigger,
                               gamepad->rightTrigger, gamepad->leftStickX, gamepad->leftStickY, gamepad->rightStickX,
                               gamepad->rightStickY);
}

void stream_input_handle_caxis(stream_input_t *input, const SDL_ControllerAxisEvent *event) {
    app_gamepad_state_t *gamepad = app_input_gamepad_state_by_instance_id(input->input, event->which);
    if (gamepad == NULL) {
        return;
    }
    switch (event->axis) {
        case SDL_CONTROLLER_AXIS_LEFTX: {
            gamepad->leftStickX = SDL_max(event->value, -32767);
            break;
        }
        case SDL_CONTROLLER_AXIS_LEFTY: {
            // Signed values have one more negative value than
            // positive value, so inverting the sign on -32768
            // could actually cause the value to overflow and
            // wrap around to be negative again. Avoid that by
            // capping the value at 32767.
            gamepad->leftStickY = (short) -SDL_max(event->value, (short) -32767);
            break;
        }
        case SDL_CONTROLLER_AXIS_RIGHTX: {
            gamepad->rightStickX = SDL_max(event->value, -32767);
            break;
        }
        case SDL_CONTROLLER_AXIS_RIGHTY: {
            gamepad->rightStickY = (short) -SDL_max(event->value, (short) -32767);
            break;
        }
        case SDL_CONTROLLER_AXIS_TRIGGERLEFT: {
            gamepad->leftTrigger = (char) (event->value * 255UL / 32767);
            break;
        }
        case SDL_CONTROLLER_AXIS_TRIGGERRIGHT: {
            gamepad->rightTrigger = (char) (event->value * 255UL / 32767);
            break;
        }
        default:
            return;
    }
    if (input->view_only) {
        return;
    }

    filter_deadzone_2axis(input, &gamepad->leftStickX, &gamepad->leftStickY);
    filter_deadzone_2axis(input, &gamepad->rightStickX, &gamepad->rightStickY);

    if (vmouse_intercepted(input, gamepad)) {
        vmouse_set_vector(&input->vmouse, gamepad->rightStickX, gamepad->rightStickY);
        vmouse_set_trigger(&input->vmouse, gamepad->leftTrigger, gamepad->rightTrigger);
        LiSendMultiControllerEvent(gamepad->gs_id, input->input->activeGamepadMask, gamepad->buttons, 0, 0,
                                   gamepad->leftStickX, gamepad->leftStickY, 0, 0);
    } else {
        LiSendMultiControllerEvent(gamepad->gs_id, input->input->activeGamepadMask, gamepad->buttons,
                                   gamepad->leftTrigger,
                                   gamepad->rightTrigger, gamepad->leftStickX, gamepad->leftStickY,
                                   gamepad->rightStickX, gamepad->rightStickY);
    }
}

void stream_input_handle_csensor(stream_input_t *input, const SDL_ControllerSensorEvent *event) {
    app_gamepad_state_t *gamepad = app_input_gamepad_state_by_instance_id(input->input, event->which);
    if (gamepad == NULL) {
        return;
    }
    if (input->view_only) {
        return;
    }
    switch (event->sensor) {
        case SDL_SENSOR_ACCEL: {
            if (sensor_state_needs_update(&gamepad->accelState, event->timestamp, event->data)) {
                gamepad->accelState.lastTimestamp = event->timestamp;
                memcpy(gamepad->accelState.data, event->data, sizeof(gamepad->accelState.data));
                LiSendControllerMotionEvent(gamepad->gs_id, LI_MOTION_TYPE_ACCEL, event->data[0], event->data[1],
                                            event->data[2]);
            }
            break;
        }
        case SDL_SENSOR_GYRO: {
            if (sensor_state_needs_update(&gamepad->gyroState, event->timestamp, event->data)) {
                gamepad->gyroState.lastTimestamp = event->timestamp;
                memcpy(gamepad->gyroState.data, event->data, sizeof(gamepad->gyroState.data));
                // Convert rad/s to deg/s
                LiSendControllerMotionEvent(gamepad->gs_id, LI_MOTION_TYPE_GYRO,
                                            event->data[0] * 57.2957795f,
                                            event->data[1] * 57.2957795f,
                                            event->data[2] * 57.2957795f);
            }
            break;
        }
        default: {
            return;
        }
    }
}

void stream_input_handle_ctouchpad(stream_input_t *input, const SDL_ControllerTouchpadEvent *event) {
    if (event->touchpad != 0 || input->view_only) {
        return;
    }

    app_gamepad_state_t *gamepad = app_input_gamepad_state_by_instance_id(input->input, event->which);
    if (gamepad == NULL) {
        return;
    }

    // Ignoring every contact past the first disables two-finger gestures wholesale,
    // whether the user turned them off or the pad only tracks one finger.
    if (event->finger != 0 && !touchpad_multitouch_enabled(input, gamepad)) {
        return;
    }

    if (input->touchpad_mode == TOUCHPAD_MODE_NATIVE) {
        uint8_t native_event_type =
                event->type == SDL_CONTROLLERTOUCHPADDOWN ? LI_TOUCH_EVENT_DOWN :
                event->type == SDL_CONTROLLERTOUCHPADUP ? LI_TOUCH_EVENT_UP :
                                                          LI_TOUCH_EVENT_MOVE;
        LiSendControllerTouchEvent(gamepad->gs_id, native_event_type, event->finger,
                                   event->x, event->y, event->pressure);
        return;
    }

    session_input_touchpad_t *state = touchpad_state(input, gamepad);
    if (state == NULL) {
        return;
    }

    touchpad_finger_t *finger = NULL;
    bool was_active = false;
    float finger_dx = 0.0f, finger_dy = 0.0f;
    if (event->finger >= 0 && event->finger < TOUCHPAD_TRACKED_FINGERS) {
        uint8_t finger_bit = (uint8_t) (1u << event->finger);
        finger = &state->fingers[event->finger];
        was_active = (state->active_fingers & finger_bit) != 0;
        if (was_active && event->type == SDL_CONTROLLERTOUCHPADMOTION) {
            finger_dx = event->x - finger->x;
            finger_dy = event->y - finger->y;
        }

        if (!was_active && event->type != SDL_CONTROLLERTOUCHPADUP) {
            finger->down_x = event->x;
            finger->down_y = event->y;
            finger->down_timestamp = event->timestamp;
        }
        finger->x = event->x;
        finger->y = event->y;

        if (was_active && state->tap_state == TOUCHPAD_TAP_SINGLE_PENDING &&
            touchpad_finger_exceeded_tap_slop(
                    finger, TOUCHPAD_SINGLE_TAP_SLOP_SQ)) {
            bool hold_elapsed = event->type == SDL_CONTROLLERTOUCHPADMOTION &&
                                SDL_TICKS_PASSED(event->timestamp,
                                                 finger->down_timestamp + TOUCHPAD_TAP_THRESHOLD_MS);
            if (hold_elapsed) {
                state->tap_state = TOUCHPAD_TAP_SINGLE_HOLD;
                LiSendMouseButtonEvent(BUTTON_ACTION_PRESS, BUTTON_LEFT);
            } else {
                touchpad_end_tap(state);
            }
        }

        if (was_active && state->tap_state == TOUCHPAD_TAP_TWO_PENDING &&
            touchpad_finger_exceeded_tap_slop(
                    finger, TOUCHPAD_TWO_FINGER_TAP_SLOP_SQ)) {
            touchpad_end_tap(state);
        }

        if (event->type == SDL_CONTROLLERTOUCHPADUP) {
            state->active_fingers &= (uint8_t) ~finger_bit;
        } else {
            state->active_fingers |= finger_bit;
        }
    } else if (event->type == SDL_CONTROLLERTOUCHPADDOWN) {
        touchpad_end_tap(state);
    }

    bool two_fingers = state->active_fingers == 0x3u;
    if (event->type == SDL_CONTROLLERTOUCHPADDOWN) {
        if (two_fingers) {
            touchpad_end_tap(state);

            {
                uint32_t t0 = state->fingers[0].down_timestamp;
                uint32_t t1 = state->fingers[1].down_timestamp;
                uint32_t chord_delta = t0 >= t1 ? t0 - t1 : t1 - t0;
                bool within_slop =
                        !touchpad_finger_exceeded_tap_slop(
                                &state->fingers[0], TOUCHPAD_TWO_FINGER_TAP_SLOP_SQ) &&
                        !touchpad_finger_exceeded_tap_slop(
                                &state->fingers[1], TOUCHPAD_TWO_FINGER_TAP_SLOP_SQ);
                if (chord_delta <= TOUCHPAD_TWO_FINGER_TAP_CHORD_MS && within_slop) {
                    state->tap_state = TOUCHPAD_TAP_TWO_PENDING;
                }
            }
        } else if (state->active_fingers != 0 && finger != NULL && state->physical_mouse_button == 0) {
            state->tap_state = TOUCHPAD_TAP_SINGLE_PENDING;
        }
    } else if (event->type == SDL_CONTROLLERTOUCHPADUP) {
        if (finger != NULL && state->tap_state == TOUCHPAD_TAP_SINGLE_HOLD) {
            touchpad_end_tap(state);
        } else if (finger != NULL && state->tap_state == TOUCHPAD_TAP_SINGLE_PENDING) {
            if (event->timestamp - finger->down_timestamp <= TOUCHPAD_TAP_THRESHOLD_MS) {
                touchpad_send_mouse_click(BUTTON_LEFT);
            }
            touchpad_end_tap(state);
        }

        if (state->tap_state == TOUCHPAD_TAP_TWO_PENDING && state->active_fingers == 0) {
            uint32_t tap_start = state->fingers[0].down_timestamp < state->fingers[1].down_timestamp
                                 ? state->fingers[0].down_timestamp
                                 : state->fingers[1].down_timestamp;
            if (event->timestamp - tap_start <= TOUCHPAD_TAP_THRESHOLD_MS) {
                touchpad_send_mouse_click(BUTTON_RIGHT);
            }
            touchpad_end_tap(state);
        }
    }

    // Keep pointer/scroll motion quiet until a two-finger tap either completes
    // or exceeds its movement threshold.
    if (state->tap_state == TOUCHPAD_TAP_TWO_PENDING && state->active_fingers != 0) {
        state->motion_state = TOUCHPAD_MOTION_NONE;
        return;
    }

    if (two_fingers) {
        if (state->motion_state != TOUCHPAD_MOTION_SCROLL ||
            event->type != SDL_CONTROLLERTOUCHPADMOTION) {
            state->motion_state = TOUCHPAD_MOTION_SCROLL;
            state->motion_remainder_x = 0.0f;
            state->motion_remainder_y = 0.0f;
            return;
        }

        // Each SDL event reports one contact's movement. Halving each
        // contribution makes the shared accumulator track the two-finger
        // centroid instead of summing both contacts and doubling sensitivity.
        state->motion_remainder_x += finger_dx * 0.5f * input->touchpad_scroll_scale;
        state->motion_remainder_y += finger_dy * 0.5f * input->touchpad_scroll_scale;

        short scroll_x = touchpad_take_delta(&state->motion_remainder_x);
        short scroll_y = touchpad_take_delta(&state->motion_remainder_y);
        if (scroll_y != 0) {
            LiSendHighResScrollEvent(scroll_y);
        }
        if (scroll_x != 0) {
            LiSendHighResHScrollEvent(scroll_x);
        }
        return;
    }

    if (state->motion_state == TOUCHPAD_MOTION_SCROLL) {
        // Keep the pointer frozen until every finger from the scroll gesture is up.
        if (state->active_fingers != 0) {
            return;
        }

        state->motion_state = TOUCHPAD_MOTION_NONE;
        state->motion_remainder_x = 0.0f;
        state->motion_remainder_y = 0.0f;
        return;
    }

    int primary_finger = touchpad_primary_finger(state);
    if (primary_finger < 0) {
        state->motion_state = TOUCHPAD_MOTION_NONE;
        return;
    }

    uint8_t pointer_state = (uint8_t) (TOUCHPAD_MOTION_POINTER_0 + primary_finger);
    if (state->motion_state != pointer_state || event->type != SDL_CONTROLLERTOUCHPADMOTION) {
        state->motion_state = pointer_state;
        state->motion_remainder_x = 0.0f;
        state->motion_remainder_y = 0.0f;
        return;
    }

    if (event->finger != primary_finger) {
        return;
    }

    float gain = input->touchpad_mouse_gain;
    state->motion_remainder_x += finger_dx * gain;
    state->motion_remainder_y += finger_dy * gain * touchpad_aspect(gamepad);

    short dx = touchpad_take_delta(&state->motion_remainder_x);
    short dy = touchpad_take_delta(&state->motion_remainder_y);
    if (dx != 0 || dy != 0) {
        LiSendMouseMoveEvent(dx, dy);
    }
}

void stream_input_update_touchpad_tap_hold(stream_input_t *input) {
    uint32_t timestamp = SDL_GetTicks();
    for (short i = 0; i < input->touchpad_count; ++i) {
        session_input_touchpad_t *state = &input->touchpads[i];
        if (state->tap_state != TOUCHPAD_TAP_SINGLE_PENDING) {
            continue;
        }
        int primary_finger = touchpad_primary_finger(state);
        if (primary_finger < 0) {
            touchpad_end_tap(state);
            continue;
        }

        const touchpad_finger_t *finger = &state->fingers[primary_finger];
        if (touchpad_finger_exceeded_tap_slop(
                    finger, TOUCHPAD_SINGLE_TAP_SLOP_SQ)) {
            touchpad_end_tap(state);
            continue;
        }

        if (SDL_TICKS_PASSED(
                    timestamp,
                    finger->down_timestamp + TOUCHPAD_TAP_THRESHOLD_MS)) {
            state->tap_state = TOUCHPAD_TAP_SINGLE_HOLD;
            LiSendMouseButtonEvent(BUTTON_ACTION_PRESS, BUTTON_LEFT);
        }
    }
}

void stream_input_handle_cdevice(stream_input_t *input, const SDL_ControllerDeviceEvent *event) {
    app_gamepad_state_t *gamepad = app_input_gamepad_state_by_instance_id(input->input, event->which);
    if (gamepad == NULL) {
        return;
    }
    if (input->view_only) {
        return;
    }
    stream_input_send_gamepad_arrive(input, gamepad);
}

void stream_input_send_gamepad_arrive(const stream_input_t *input, app_gamepad_state_t *gamepad) {
    uint8_t type = LI_CTYPE_XBOX;
    uint16_t capabilities = LI_CCAP_ANALOG_TRIGGERS;
    commons_log_info("Input", "Controller %d arrived. Name: %s", gamepad->gs_id,
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
            break;
        }
        default: {
            break;
        }
    }
    // Reported from the hardware rather than the active mode: the pad exists either
    // way, we simply keep its events to ourselves while it is acting as a mouse.
    if (controller_has_touchpad(gamepad)) {
        capabilities |= LI_CCAP_TOUCHPAD;
        commons_log_info("Input", "  controller capability: touchpad");
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
    LiSendControllerArrivalEvent(gamepad->gs_id, input->input->activeGamepadMask, type,
                                 0xFFFFFFFF, capabilities);
}

static int touchpad_primary_finger(const session_input_touchpad_t *state) {
    return state->active_fingers & 1u ? 0 : (state->active_fingers & 2u ? 1 : -1);
}

static bool touchpad_lower_right_press(const session_input_touchpad_t *state) {
    int primary_finger = touchpad_primary_finger(state);
    if (primary_finger < 0) {
        return false;
    }

    const touchpad_finger_t *finger = &state->fingers[primary_finger];
    return finger->x >= TOUCHPAD_SECONDARY_CORNER &&
           finger->y >= TOUCHPAD_SECONDARY_CORNER;
}

static bool touchpad_finger_exceeded_tap_slop(
        const touchpad_finger_t *finger, float threshold_squared) {
    float dx = finger->x - finger->down_x;
    float dy = finger->y - finger->down_y;
    return dx * dx + dy * dy > threshold_squared;
}

static void touchpad_end_tap(session_input_touchpad_t *state) {
    if (state->tap_state == TOUCHPAD_TAP_SINGLE_HOLD) {
        LiSendMouseButtonEvent(BUTTON_ACTION_RELEASE, BUTTON_LEFT);
    }
    state->tap_state = TOUCHPAD_TAP_NONE;
}

static void touchpad_send_mouse_click(int mouse_button) {
    if (mouse_button == 0) {
        return;
    }
    LiSendMouseButtonEvent(BUTTON_ACTION_PRESS, mouse_button);
    LiSendMouseButtonEvent(BUTTON_ACTION_RELEASE, mouse_button);
}

static short touchpad_take_delta(float *remainder) {
    // SDL touch coordinates are normalized, so scaled per-event deltas stay
    // comfortably within Limelight's signed 16-bit mouse/scroll range.
    short result = (short) *remainder;
    *remainder -= (float) result;
    return result;
}

static session_input_touchpad_t *touchpad_state(
        stream_input_t *input, const app_gamepad_state_t *gamepad) {
    if (input->touchpads == NULL || gamepad->gs_id < 0 ||
        gamepad->gs_id >= input->touchpad_count) {
        return NULL;
    }
    return &input->touchpads[gamepad->gs_id];
}

static void touchpad_reset_state(session_input_touchpad_t *state) {
    touchpad_end_tap(state);
    if (state->physical_mouse_button > 0) {
        LiSendMouseButtonEvent(BUTTON_ACTION_RELEASE, state->physical_mouse_button);
    }
    SDL_memset(state, 0, sizeof(*state));
}

static bool controller_has_touchpad(const app_gamepad_state_t *gamepad) {
    if (gamepad->controller == NULL) {
        return false;
    }

    SDL_GameControllerType controller_type = SDL_GameControllerGetType(gamepad->controller);
    if (controller_type == SDL_CONTROLLER_TYPE_PS4 ||
        controller_type == SDL_CONTROLLER_TYPE_PS5) {
        return true;
    }

#if SDL_VERSION_ATLEAST(2, 0, 14)
    return SDL_GameControllerGetNumTouchpads(gamepad->controller) > 0;
#else
    return false;
#endif
}

static bool touchpad_multitouch_enabled(const stream_input_t *input, const app_gamepad_state_t *gamepad) {
    if (!input->touchpad_multitouch || gamepad->controller == NULL) {
        return false;
    }
#if SDL_VERSION_ATLEAST(2, 0, 14)
    return SDL_GameControllerGetNumTouchpadFingers(gamepad->controller, 0) >= 2;
#else
    return false;
#endif
}

/* Vertical travel per unit of normalised movement, relative to horizontal.
 *
 * SDL normalises touchpad coordinates to 0..1 by dividing out each pad's own
 * extents, which discards the aspect ratio, so scaling both axes equally makes
 * vertical movement run fast on any pad that isn't 16:9. The ratios below are
 * the divisors SDL itself uses -- see TOUCHPAD_SCALEX/TOUCHPAD_SCALEY in
 * SDL_hidapi_ps4.c, SDL_hidapi_ps5.c and SDL_hidapi_shield.c, which are the only
 * drivers that report a touchpad at all. */
static float touchpad_aspect(const app_gamepad_state_t *gamepad) {
    if (gamepad->controller != NULL) {
        switch (SDL_GameControllerGetType(gamepad->controller)) {
            case SDL_CONTROLLER_TYPE_PS4:
                return 920.0f / 1920.0f;
            case SDL_CONTROLLER_TYPE_PS5:
                return 1070.0f / 1920.0f;
#if SDL_VERSION_ATLEAST(2, 24, 0)
            case SDL_CONTROLLER_TYPE_NVIDIA_SHIELD:
                return 21.0f / 80.0f;
#endif
            default:
                break;
        }
    }
    return 9.0f / 16.0f;
}

static void release_buttons(stream_input_t *input, app_gamepad_state_t *gamepad) {
    gamepad->buttons = 0;
    gamepad->leftTrigger = 0;
    gamepad->rightTrigger = 0;
    gamepad->leftStickX = 0;
    gamepad->leftStickY = 0;
    gamepad->rightStickX = 0;
    gamepad->rightStickY = 0;
    LiSendMultiControllerEvent(gamepad->gs_id, input->input->activeGamepadMask, gamepad->buttons, gamepad->leftTrigger,
                               gamepad->rightTrigger, gamepad->leftStickX, gamepad->leftStickY, gamepad->rightStickX,
                               gamepad->rightStickY);
}


static bool gamepad_combo_check(int buttons, short combo) {
    return (buttons & combo) == combo;
}

static bool sensor_state_needs_update(const app_gamepad_sensor_state_t *state, uint32_t timestamp,
                                      const float data[3]) {
    if (state->periodMs == 0 || !SDL_TICKS_PASSED(timestamp, state->lastTimestamp + state->periodMs)) {
        return false;
    }
    for (int i = 0; i < 3; i++) {
        if (state->data[i] != data[i]) {
            return true;
        }
    }
    return false;
}

static bool vmouse_intercepted(stream_input_t *input, const app_gamepad_state_t *gamepad) {
    if (!session_input_is_vmouse_active(&input->vmouse)) {
        return false;
    }
    return gamepad->rightStickX != 0 || gamepad->rightStickY != 0 || gamepad->leftTrigger != 0 ||
           gamepad->rightTrigger != 0;
}

static bool filter_deadzone_2axis(stream_input_t *input, short *x, short *y) {
    uint32_t magnitude_pow2 = (uint32_t) (*x) * (*x) + (uint32_t) (*y) * (*y);
    uint32_t threshold_sqrt = 32768 * input->stick_deadzone / 100;
    if (magnitude_pow2 < threshold_sqrt * threshold_sqrt) {
        *x = 0;
        *y = 0;
        return true;
    }
    return false;
}