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

typedef struct stream_input_t {
    session_t *session;
    app_input_t *input;
    bool started;
    bool view_only, no_sdl_mouse;
    uint8_t stick_deadzone;
    session_input_vmouse_t vmouse;
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

void stream_input_send_gamepad_arrive(const stream_input_t *input, app_gamepad_state_t *gamepad);

void stream_input_handle_key(stream_input_t *input, const SDL_KeyboardEvent *event);

void stream_input_handle_text(stream_input_t *input, const SDL_TextInputEvent *event);

void stream_input_handle_cbutton(stream_input_t *input, const SDL_ControllerButtonEvent *event);

void stream_input_handle_caxis(stream_input_t *input, const SDL_ControllerAxisEvent *event);

void stream_input_handle_csensor(stream_input_t *input, const SDL_ControllerSensorEvent *event);

void stream_input_handle_ctouchpad(stream_input_t *input, const SDL_ControllerTouchpadEvent *event);

void stream_input_handle_cdevice(stream_input_t *input, const SDL_ControllerDeviceEvent *event);

void stream_input_handle_mmotion(stream_input_t *input, const SDL_MouseMotionEvent *event);

void stream_input_handle_mbutton(stream_input_t *input, const SDL_MouseButtonEvent *event);

void stream_input_handle_mwheel(stream_input_t *input, const SDL_MouseWheelEvent *event);

void stream_input_handle_touch(const stream_input_t *input, const SDL_TouchFingerEvent *event);