#pragma once

#include <stdbool.h>
#include <Limelight.h>
#include <SDL_events.h>
#include <SDL_timer.h>

#include "config.h"
#include "input/input_gamepad.h"

#if FEATURE_INPUT_EVMOUSE

#include "session_evmouse.h"

#endif

typedef struct app_input_t app_input_t;
typedef struct session_config_t session_config_t;
typedef struct session_t session_t;

typedef struct session_input_vmouse_t {
    struct {
        bool active;
        short x, y;
        bool l, r;
        bool modifier;
    } state;
    SDL_TimerID timer_id;
} session_input_vmouse_t;

typedef struct session_input_controller_touchpad_t session_input_controller_touchpad_t;

#if TARGET_WEBOS
typedef struct session_input_controller_touchpad_compat_t session_input_controller_touchpad_compat_t;
#endif

typedef struct stream_input_t {
    session_t *session;
    app_input_t *input;
    bool started;
    bool view_only, no_sdl_mouse;
    bool swap_abxy;
    uint8_t controller_touchpad_mode;
    uint8_t controller_touchpad_press_button;
    uint8_t controller_touchpad_secondary_button;
    bool controller_touchpad_tap_to_click;
    bool controller_touchpad_two_finger_scroll;
#if TARGET_WEBOS
    bool controller_touchpad_webos_touchscreen;
#endif
    short controller_touchpad_count;
    uint16_t announced_gamepad_mask;
#if TARGET_WEBOS
    uint16_t touchpad_gamepad_mask;
#endif
    uint32_t stick_deadzone_squared;
    float controller_touchpad_mouse_scale_x;
    float controller_touchpad_mouse_scale_y;
    float controller_touchpad_scroll_scale;
    session_input_vmouse_t vmouse;
    session_input_controller_touchpad_t *controller_touchpads;
#if TARGET_WEBOS
    session_input_controller_touchpad_compat_t *controller_touchpad_compat;
#endif
#if FEATURE_INPUT_EVMOUSE
    session_evmouse_t evmouse;
#endif
} stream_input_t;

void session_input_init(stream_input_t *input, session_t *session, app_input_t *app_input,
                        const session_config_t *settings);

void session_input_deinit(stream_input_t *input);

void session_input_interrupt(stream_input_t *input);

void session_input_started(stream_input_t *input);

void session_input_stopped(stream_input_t *input);

void stream_input_controller_touchpad_mouse_init(stream_input_t *input);

void stream_input_controller_touchpad_mouse_deinit(stream_input_t *input);

#if TARGET_WEBOS
void stream_input_controller_touchpad_compat_deinit(stream_input_t *input);

void stream_input_controller_touchpad_compat_set_present(stream_input_t *input, bool present);

bool stream_input_controller_touchpad_compat_mouse_active(const stream_input_t *input, uint32_t timestamp);

void stream_input_controller_touchpad_compat_observe(stream_input_t *input,
                                                     const SDL_ControllerTouchpadEvent *event);

void stream_input_flush_pending_touch(stream_input_t *input);
#endif

void session_input_screen_keyboard_opened(stream_input_t *input);

void session_input_screen_keyboard_closed(stream_input_t *input);

void stream_input_sync_gamepads(stream_input_t *input);

void stream_input_handle_key(stream_input_t *input, const SDL_KeyboardEvent *event);

void stream_input_handle_text(stream_input_t *input, const SDL_TextInputEvent *event);

void stream_input_handle_cbutton(stream_input_t *input, const SDL_ControllerButtonEvent *event);

void stream_input_handle_caxis(stream_input_t *input, const SDL_ControllerAxisEvent *event);

void stream_input_handle_csensor(stream_input_t *input, const SDL_ControllerSensorEvent *event);

void stream_input_handle_ctouchpad(stream_input_t *input, const SDL_ControllerTouchpadEvent *event);

void stream_input_flush_controller_touchpad_tap_hold(stream_input_t *input);

void stream_input_handle_mmotion(stream_input_t *input, const SDL_MouseMotionEvent *event, bool hw_mouse);

void stream_input_handle_mbutton(stream_input_t *input, const SDL_MouseButtonEvent *event);

void stream_input_handle_mwheel(stream_input_t *input, const SDL_MouseWheelEvent *event);

void stream_input_handle_touch(stream_input_t *input, const SDL_TouchFingerEvent *event);