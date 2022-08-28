#include "app.h"

#include "sdlinput.h"
#include "absinput.h"

#include "ui/root.h"

#include "util/logging.h"

#if TARGET_WEBOS

#include <SDL_webOS.h>

#endif


static void release_keyboard_keys(SDL_Event ev);

static void sdlinput_handle_input_result(SDL_Event ev, int ret);

static bool absinput_virtual_mouse;

void sdlinput_handle_key_event(SDL_KeyboardEvent *event);

void sdlinput_handle_text_event(SDL_TextInputEvent *event);

void sdlinput_handle_cbutton_event(SDL_ControllerButtonEvent *event);

void sdlinput_handle_caxis_event(SDL_ControllerAxisEvent *event);

void sdlinput_handle_mmotion_event(SDL_MouseMotionEvent *event);

bool absinput_started;
bool absinput_no_control;
bool absinput_no_sdl_mouse;

GAMEPAD_STATE gamepads[4];
short activeGamepadMask = 0;
int sdl_gamepads = 0;

void absinput_init() {
    memset(gamepads, 0, sizeof(gamepads));
    absinput_started = false;
}

void absinput_destroy() {
}

int absinput_gamepads() {
    return sdl_gamepads;
}

int absinput_max_gamepads() {
    return 4;
}

bool absinput_gamepad_present(int which) {
    return (activeGamepadMask & (1 << which)) != 0;
}

void absinput_rumble(unsigned short controller_id, unsigned short low_freq_motor, unsigned short high_freq_motor) {
    if (controller_id >= 4)
        return;

    PGAMEPAD_STATE state = &gamepads[controller_id];

    SDL_Haptic *haptic = state->haptic;
    if (!haptic)
        return;

    if (state->haptic_effect_id >= 0)
        SDL_HapticDestroyEffect(haptic, state->haptic_effect_id);

    if (low_freq_motor == 0 && high_freq_motor == 0)
        return;

    SDL_HapticEffect effect;
    SDL_memset(&effect, 0, sizeof(effect));
    effect.type = SDL_HAPTIC_LEFTRIGHT;
    effect.leftright.length = SDL_HAPTIC_INFINITY;

    // SDL haptics range from 0-32767 but XInput uses 0-65535, so divide by 2 to correct for SDL's scaling
    effect.leftright.large_magnitude = low_freq_motor / 2;
    effect.leftright.small_magnitude = high_freq_motor / 2;

    state->haptic_effect_id = SDL_HapticNewEffect(haptic, &effect);
    if (state->haptic_effect_id >= 0)
        SDL_HapticRunEffect(haptic, state->haptic_effect_id, 1);
}

bool absinput_should_accept() {
    return absinput_started && !ui_should_block_input();
}

void absinput_start() {
    absinput_started = true;
}

void absinput_stop() {
    absinput_started = false;
}

bool absinput_dispatch_event(SDL_Event *event) {
    if (!absinput_should_accept()) {
        return false;
    }
    switch (event->type) {
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            sdlinput_handle_key_event(&event->key);
            break;
        case SDL_CONTROLLERAXISMOTION:
            sdlinput_handle_caxis_event(&event->caxis);
            break;
        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP:
            sdlinput_handle_cbutton_event(&event->cbutton);
            break;
        case SDL_MOUSEMOTION:
            sdlinput_handle_mmotion_event(&event->motion);
            break;
        case SDL_MOUSEWHEEL:
            if (!absinput_no_control && !absinput_no_sdl_mouse) {
                sdlinput_handle_mwheel_event(&event->wheel);
            }
            break;
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
            if (!absinput_no_control && !absinput_no_sdl_mouse) {
                sdlinput_handle_mbutton_event(&event->button);
            }
            break;
        case SDL_TEXTINPUT:
            sdlinput_handle_text_event(&event->text);
            break;
        default:
            return false;
    }
    return true;
}

void release_keyboard_keys(SDL_Event ev) {
}

bool absinput_init_gamepad(int joystick_index) {
    SDL_JoystickGUID guid = SDL_JoystickGetDeviceGUID(joystick_index);
    char guidstr[33];
    SDL_JoystickGetGUIDString(guid, guidstr, 33);
    const char *name = SDL_JoystickNameForIndex(joystick_index);
    if (SDL_IsGameController(joystick_index)) {
        SDL_GameController *controller = SDL_GameControllerOpen(joystick_index);
        if (!controller) {
            applog_e("Input", "Could not open gamecontroller %i. GUID: %s, error: %s", joystick_index, guidstr,
                     SDL_GetError());
            return false;
        }

        SDL_Joystick *joystick = SDL_GameControllerGetJoystick(controller);
        SDL_JoystickID sdl_id = SDL_JoystickInstanceID(joystick);
        PGAMEPAD_STATE state = get_gamepad(sdl_id);

        SDL_Haptic *haptic = SDL_HapticOpenFromJoystick(joystick);
        unsigned int haptic_bits = SDL_HapticQuery(haptic);
        applog_d("Input", "Controller #%d has supported haptic bits: %lx", state->id, haptic_bits);
        if (haptic && (haptic_bits & SDL_HAPTIC_LEFTRIGHT) == 0) {
            SDL_HapticClose(haptic);
            haptic = NULL;
        }

        if (state->controller) {
            applog_i("Input", "Controller #%d (%s) already connected, sdl_id: %d, GUID: %s", state->id,
                     SDL_JoystickName(joystick), sdl_id, guidstr);
            return false;
        }
        state->controller = controller;
        state->haptic = haptic;
        state->haptic_effect_id = -1;
        applog_i("Input", "Controller #%d (%s) connected, sdl_id: %d, GUID: %s", state->id, SDL_JoystickName(joystick),
                 sdl_id, guidstr);
        sdl_gamepads++;
        return true;
    } else {
        applog_w("Input", "Unrecognized game controller %s. GUID: %s", name, guidstr);
    }
    return false;
}

void absinput_close_gamepad(SDL_JoystickID sdl_id) {
    PGAMEPAD_STATE state = get_gamepad(sdl_id);
    if (!state) {
        return;
    }
    if (!state->controller) {
        applog_w("Input", "Could not find gamecontroller %i: %s", sdl_id, SDL_GetError());
        return;
    }
    // Reduce number of connected gamepads
    sdl_gamepads--;
    // Remove gamepad mask
    activeGamepadMask &= ~(1 << state->id);
    if (state->haptic) {
        SDL_HapticClose(state->haptic);
    }
    SDL_GameControllerClose(state->controller);
    applog_i("Input", "Controller #%d disconnected, sdl_id: %d", state->id, sdl_id);
    // Release the state so it can be reused later
    memset(state, 0, sizeof(GAMEPAD_STATE));
}

bool absinput_gamepad_known(SDL_JoystickID sdl_id) {
    for (short i = 0; i < 4; i++) {
        if (gamepads[i].initialized && gamepads[i].sdl_id == sdl_id) return true;
    }
    return false;
}

void absinput_set_virtual_mouse(bool enabled) {
    absinput_virtual_mouse = app_configuration->virtual_mouse && enabled;
}

bool absinput_get_virtual_mouse() {
    return absinput_virtual_mouse;
}