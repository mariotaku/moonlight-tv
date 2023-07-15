#pragma once

#include <SDL_assert.h>

typedef struct app_t app_t;

void app_fatal_error(const char *title, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
