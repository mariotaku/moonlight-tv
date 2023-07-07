#pragma once

#include <SDL_assert.h>

typedef struct app_t app_t;

SDL_AssertState app_assertion_handler_abort(const SDL_AssertData *data, void *userdata);

_Noreturn void app_halt(app_t *app);
