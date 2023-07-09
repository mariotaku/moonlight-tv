#pragma once

#include <stdbool.h>
#include <Limelight.h>
#include <SDL_events.h>

typedef struct app_input_t app_input_t;
typedef struct session_t session_t;

typedef struct stream_input_t {
    session_t*session;
    app_input_t *input;
} stream_input_t;

extern bool absinput_no_control;
extern bool absinput_no_sdl_mouse;

void absinput_start();

void stream_input_stop();

void absinput_set_virtual_mouse(bool enabled);

bool absinput_get_virtual_mouse();

void sdlinput_handle_key_event(stream_input_t *input,const SDL_KeyboardEvent *event);

void sdlinput_handle_text_event(stream_input_t *input,const SDL_TextInputEvent *event);

void stream_input_handle_cbutton(stream_input_t *input, const SDL_ControllerButtonEvent *event);

void sdlinput_handle_caxis_event(stream_input_t *input, const SDL_ControllerAxisEvent *event);

void session_handle_mmotion_event(stream_input_t *input, const SDL_MouseMotionEvent *event);
