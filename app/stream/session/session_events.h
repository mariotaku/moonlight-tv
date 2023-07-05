#pragma once

#include <stdbool.h>
#include <SDL_events.h>

typedef struct session_t session_t;

bool session_input_handle_event(session_t *session, const SDL_Event *event);
