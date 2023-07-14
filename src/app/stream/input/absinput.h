#pragma once

#include <stdbool.h>
#include <Limelight.h>
#include <SDL_events.h>
#include <SDL_timer.h>

typedef struct app_input_t app_input_t;
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
    bool virtual_mouse;
    session_input_vmouse_t vmouse;
} stream_input_t;

void sdlinput_handle_key_event(stream_input_t *input, const SDL_KeyboardEvent *event);

void sdlinput_handle_text_event(stream_input_t *input, const SDL_TextInputEvent *event);

void stream_input_handle_cbutton(stream_input_t *input, const SDL_ControllerButtonEvent *event);

void sdlinput_handle_caxis_event(stream_input_t *input, const SDL_ControllerAxisEvent *event);

void session_handle_mmotion_event(stream_input_t *input, const SDL_MouseMotionEvent *event);

void sdlinput_handle_mbutton_event(const SDL_MouseButtonEvent *event);

void sdlinput_handle_mwheel_event(const SDL_MouseWheelEvent *event);