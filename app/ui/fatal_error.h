#pragma once

void app_fatal_error(const char *title, const char *fmt, ...) __attribute__((format(printf, 2, 3)));

_Noreturn void app_halt();