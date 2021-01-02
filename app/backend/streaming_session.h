#pragma once

#include <stdbool.h>

void streaming_init();

void streaming_destroy();

void streaming_begin(const char *addr, int app_id);

void streaming_interrupt();

void streaming_wait_for_stop();

bool streaming_running();