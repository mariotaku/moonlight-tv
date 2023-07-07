#pragma once

#include <SDL_assert.h>

void app_fatal_error(const char *title, const char *fmt, ...) __attribute__((format(printf, 2, 3)));

_Noreturn void app_halt();

SDL_AssertState app_assertion_handler_abort(const SDL_AssertData *data, void *userdata);