#include "input_gamepad.h"

#include <stdbool.h>

#include <SDL_gamecontroller.h>
#include <SDL_version.h>
#include <Limelight.h>

#include "logging.h"
#include "app_input.h"

bool app_input_init_gamepad(app_input_t *input, int which) {
    SDL_JoystickGUID guid = SDL_JoystickGetDeviceGUID(which);
    char guidstr[33];
    SDL_JoystickGetGUIDString(guid, guidstr, 33);
    const char *name = SDL_JoystickNameForIndex(which);
    if (SDL_IsGameController(which)) {
        SDL_GameController *controller = SDL_GameControllerOpen(which);
        if (!controller) {
            commons_log_error("Input", "Could not open gamecontroller %i. GUID: %s, error: %s", which, guidstr,
                              SDL_GetError());
            return false;
        }

        SDL_Joystick *joystick = SDL_GameControllerGetJoystick(controller);
        SDL_JoystickID sdl_id = SDL_JoystickInstanceID(joystick);
        app_gamepad_state_t *state = app_input_gamepad_get(input, sdl_id);

        SDL_Haptic *haptic = SDL_HapticOpenFromJoystick(joystick);
        unsigned int haptic_bits = SDL_HapticQuery(haptic);
        commons_log_debug("Input", "Controller #%d has supported haptic bits: %x", state->id, haptic_bits);
        if (haptic && (haptic_bits & SDL_HAPTIC_LEFTRIGHT) == 0) {
            SDL_HapticClose(haptic);
            haptic = NULL;
        }

        if (state->controller) {
            commons_log_info("Input", "Controller #%d (%s) already connected, sdl_id: %d, GUID: %s", state->id,
                             SDL_JoystickName(joystick), sdl_id, guidstr);
            return false;
        }
        state->controller = controller;
#if !SDL_VERSION_ATLEAST(2, 0, 9)
        state->haptic = haptic;
        state->haptic_effect_id = -1;
#endif
        commons_log_info("Input", "Controller #%d (%s) connected, sdl_id: %d, path: %s, GUID: %s", state->id,
                         SDL_JoystickName(joystick), sdl_id, SDL_GameControllerPath(controller), guidstr);
        input->gamepads_count++;
        return true;
    } else {
        commons_log_warn("Input", "Unrecognized game controller %s. GUID: %s", name, guidstr);
    }
    return false;
}

void app_input_close_gamepad(app_input_t *input, SDL_JoystickID sdl_id) {
    app_gamepad_state_t *state = app_input_gamepad_get(input, sdl_id);
    if (!state) {
        return;
    }
    if (!state->controller) {
        commons_log_warn("Input", "Could not find gamecontroller %i: %s", sdl_id, SDL_GetError());
        return;
    }
    // Reduce number of connected gamepads
    input->gamepads_count--;
    // Remove gamepad mask
    input->activeGamepadMask &= ~(1 << state->id);
#if !SDL_VERSION_ATLEAST(2, 0, 9)
    if (state->haptic) {
        SDL_HapticClose(state->haptic);
    }
#endif
    SDL_GameControllerClose(state->controller);
    commons_log_info("Input", "Controller #%d disconnected, sdl_id: %d", state->id, sdl_id);
    // Release the state so it can be reused later
    memset(state, 0, sizeof(app_gamepad_state_t));
}

bool absinput_gamepad_known(app_input_t *input, SDL_JoystickID sdl_id) {
    for (short i = 0; i < 4; i++) {
        if (input->gamepads[i].initialized && input->gamepads[i].sdl_id == sdl_id) { return true; }
    }
    return false;
}

app_gamepad_state_t *app_input_gamepad_get(app_input_t *input, SDL_JoystickID sdl_id) {
    for (short i = 0; i < input->max_num_gamepads; i++) {
        app_gamepad_state_t *gamepad = &input->gamepads[i];
        if (!input->gamepads[i].initialized) {
            input->gamepads[i].sdl_id = sdl_id;
            input->gamepads[i].id = i;
            input->gamepads[i].initialized = true;
            input->activeGamepadMask |= (1 << i);
            return gamepad;
        } else if (gamepad->sdl_id == sdl_id) {
            return gamepad;
        }
    }
    return &input->gamepads[0];
}

int app_input_gamepads(app_input_t *input) {
    return input->gamepads_count;
}

int app_input_max_gamepads(app_input_t *input) {
    return input->max_num_gamepads;
}

int app_input_gamepads_mask(app_input_t *input) {
    return input->activeGamepadMask;
}

bool app_input_gamepad_present(app_input_t *input, int which) {
    return (input->activeGamepadMask & (1 << which)) != 0;
}

void app_input_gamepad_rumble(app_input_t *input, unsigned short controller_id,
                              unsigned short low_freq_motor, unsigned short high_freq_motor) {
    if (controller_id >= 4) {
        return;
    }

    app_gamepad_state_t *state = &input->gamepads[controller_id];

#if SDL_VERSION_ATLEAST(2, 0, 9)
    SDL_GameControllerRumble(state->controller, low_freq_motor, high_freq_motor, SDL_HAPTIC_INFINITY);
#else
    SDL_Haptic *haptic = state->haptic;
    if (!haptic) {
        return;
    }

    if (state->haptic_effect_id >= 0) {
        SDL_HapticDestroyEffect(haptic, state->haptic_effect_id);
    }

    if (low_freq_motor == 0 && high_freq_motor == 0) {
        return;
    }

    SDL_HapticEffect effect;
    SDL_memset(&effect, 0, sizeof(effect));
    effect.type = SDL_HAPTIC_LEFTRIGHT;
    effect.leftright.length = SDL_HAPTIC_INFINITY;

    // SDL haptics range from 0-32767 but XInput uses 0-65535, so divide by 2 to correct for SDL's scaling
    effect.leftright.large_magnitude = low_freq_motor / 2;
    effect.leftright.small_magnitude = high_freq_motor / 2;

    state->haptic_effect_id = SDL_HapticNewEffect(haptic, &effect);
    if (state->haptic_effect_id >= 0) {
        SDL_HapticRunEffect(haptic, state->haptic_effect_id, 1);
    }
#endif
}


void app_input_gamepad_rumble_triggers(app_input_t *input, unsigned short controllerNumber, unsigned short leftTrigger,
                                       unsigned short rightTrigger) {
#if SDL_VERSION_ATLEAST(2, 0, 14)
    SDL_GameControllerRumbleTriggers(input->gamepads[controllerNumber].controller, leftTrigger, rightTrigger,
                                     SDL_HAPTIC_INFINITY);
#endif
}

void app_input_gamepad_set_motion_event_state(app_input_t *input, unsigned short controllerNumber, uint8_t motionType,
                                              uint16_t reportRateHz) {
#if SDL_VERSION_ATLEAST(2, 0, 14)
    SDL_SensorType sensor_type = SDL_SENSOR_INVALID;
    switch (motionType) {
        case LI_MOTION_TYPE_ACCEL:
            sensor_type = SDL_SENSOR_ACCEL;
            break;
        case LI_MOTION_TYPE_GYRO:
            sensor_type = SDL_SENSOR_GYRO;
            break;
        default:
            break;
    }
    if (sensor_type == SDL_SENSOR_INVALID) {
        return;
    }
    SDL_GameController *controller = input->gamepads[controllerNumber].controller;
    SDL_GameControllerSetSensorEnabled(controller, sensor_type, reportRateHz > 0 ? SDL_TRUE : SDL_FALSE);
    commons_log_info("Input", "Setting motion event state for controller %d, motionType: %d, reportRateHz: %d",
                     controllerNumber, motionType, reportRateHz);
#endif
}


void app_input_gamepad_set_controller_led(app_input_t *input, unsigned short controllerNumber, uint8_t r, uint8_t g,
                                          uint8_t b) {
#if SDL_VERSION_ATLEAST(2, 0, 14)
    SDL_GameControllerSetLED(input->gamepads[controllerNumber].controller, r, g, b);
#endif
}