/*
 * Copyright (c) 2023 Ningyuan Li <https://github.com/mariotaku>.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "session_input.h"
#include "stream/session.h"

#define CONTROLLER_TOUCHPAD_MOUSE_SCALE_X_PERCENT 16.0f
#define CONTROLLER_TOUCHPAD_MOUSE_SCALE_Y_PERCENT 9.0f
#define CONTROLLER_TOUCHPAD_SCROLL_FINGER_SCALE 600.0f

static uint8_t controller_touchpad_mouse_button(int press) {
    static const uint8_t buttons[] = {0, BUTTON_LEFT, BUTTON_RIGHT, BUTTON_MIDDLE};
    return (unsigned int) press < sizeof(buttons) / sizeof(buttons[0]) ? buttons[press] : 0;
}

#if FEATURE_INPUT_EVMOUSE
static bool session_input_uses_evmouse(const stream_input_t *input) {
    return !input->view_only && input->no_sdl_mouse;
}
#endif

void session_input_init(stream_input_t *input, session_t *session, app_input_t *app_input,
                        const session_config_t *config) {
    input->session = session;
    input->input = app_input;
    input->started = false;
    input->view_only = config->view_only;
    input->no_sdl_mouse = config->hardware_mouse;
    input->swap_abxy = config->swap_abxy;
    input->controller_touchpad_mode = (uint8_t) config->controller_touchpad_mode;
    input->controller_touchpad_press_button =
            controller_touchpad_mouse_button((uint8_t) config->controller_touchpad_press);
    input->controller_touchpad_secondary_button =
            controller_touchpad_mouse_button((uint8_t) config->controller_touchpad_secondary_click);
    input->controller_touchpad_tap_to_click = config->controller_touchpad_tap_to_click;
    input->controller_touchpad_two_finger_scroll = config->controller_touchpad_two_finger_scroll;
    input->controller_touchpad_mouse_scale_x = CONTROLLER_TOUCHPAD_MOUSE_SCALE_X_PERCENT *
                                               (float) (uint8_t) config->controller_touchpad_sensitivity;
    input->controller_touchpad_mouse_scale_y = CONTROLLER_TOUCHPAD_MOUSE_SCALE_Y_PERCENT *
                                               (float) (uint8_t) config->controller_touchpad_sensitivity;
    input->controller_touchpad_scroll_scale = config->controller_touchpad_invert_two_finger_scroll
                                              ? CONTROLLER_TOUCHPAD_SCROLL_FINGER_SCALE
                                              : -CONTROLLER_TOUCHPAD_SCROLL_FINGER_SCALE;
    input->announced_gamepad_mask = 0;
#if TARGET_WEBOS
    input->touchpad_gamepad_mask = 0;
#endif
    uint32_t stick_deadzone = 32768u * (uint32_t) config->stick_deadzone / 100u;
    input->stick_deadzone_squared = stick_deadzone * stick_deadzone;
#if TARGET_WEBOS
    input->controller_touchpad_webos_touchscreen = config->controller_touchpad_webos_touchscreen;
    input->controller_touchpad_compat = NULL;
#endif

    input->controller_touchpads = NULL;
    input->controller_touchpad_count = 0;
#if FEATURE_INPUT_EVMOUSE
    if (session_input_uses_evmouse(input)) {
        session_evmouse_init(&input->evmouse, session);
    }
#endif
}

void session_input_deinit(stream_input_t *input) {
#if TARGET_WEBOS
    stream_input_controller_touchpad_compat_deinit(input);
#endif
    stream_input_controller_touchpad_mouse_deinit(input);
#if FEATURE_INPUT_EVMOUSE
    if (session_input_uses_evmouse(input)) {
        session_evmouse_deinit(&input->evmouse);
    }
#endif
}

void session_input_interrupt(stream_input_t *input) {
#if FEATURE_INPUT_EVMOUSE
    if (session_input_uses_evmouse(input)) {
        session_evmouse_interrupt(&input->evmouse);
    }
#endif
}

void session_input_started(stream_input_t *input) {
    input->started = true;
    input->announced_gamepad_mask = 0;
#if TARGET_WEBOS
    input->touchpad_gamepad_mask = 0;
#endif
    if (!input->view_only) {
        stream_input_controller_touchpad_mouse_init(input);
        stream_input_sync_gamepads(input);
    }
}

void session_input_stopped(stream_input_t *input) {
    input->started = false;
    input->announced_gamepad_mask = 0;
#if TARGET_WEBOS
    input->touchpad_gamepad_mask = 0;
#endif
    stream_input_controller_touchpad_mouse_deinit(input);
#if TARGET_WEBOS
    stream_input_controller_touchpad_compat_deinit(input);
#endif
}

void session_input_screen_keyboard_opened(stream_input_t *input) {
#if FEATURE_INPUT_EVMOUSE
    if (session_input_uses_evmouse(input)) {
        session_evmouse_disable(&input->evmouse);
    }
#endif
}

void session_input_screen_keyboard_closed(stream_input_t *input) {
#if FEATURE_INPUT_EVMOUSE
    if (session_input_uses_evmouse(input)) {
        session_evmouse_enable(&input->evmouse);
    }
#endif
}