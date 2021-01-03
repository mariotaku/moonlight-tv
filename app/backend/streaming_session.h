#pragma once

#include <stdbool.h>

enum STREAMING_STATUS
{
    STREAMING_NONE, STREAMING_CONNECTING, STREAMING_STREAMING, STREAMING_DISCONNECTING
};
typedef enum STREAMING_STATUS STREAMING_STATUS;

void streaming_init();

void streaming_destroy();

void streaming_begin(const char *addr, int app_id);

void streaming_interrupt();

void streaming_wait_for_stop();

bool streaming_running();

STREAMING_STATUS streaming_status();