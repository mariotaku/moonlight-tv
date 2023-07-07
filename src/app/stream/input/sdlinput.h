#pragma once

#include <stdbool.h>

#include <SDL.h>

typedef struct stream_input_t stream_input_t;

bool stream_input_dispatch_event(stream_input_t *input, SDL_Event *event);

bool session_input_should_accept(stream_input_t *input);


void sdlinput_handle_mbutton_event(const SDL_MouseButtonEvent *event);

void sdlinput_handle_mwheel_event(const SDL_MouseWheelEvent *event);