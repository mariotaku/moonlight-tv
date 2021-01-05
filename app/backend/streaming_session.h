#pragma once

#include <stdbool.h>
#include <SDL.h>

#include "computer_manager.h"

enum STREAMING_STATUS
{
    STREAMING_NONE, STREAMING_CONNECTING, STREAMING_STREAMING, STREAMING_DISCONNECTING
};
typedef enum STREAMING_STATUS STREAMING_STATUS;

extern STREAMING_STATUS streaming_status;

void streaming_init();

void streaming_destroy();

void streaming_begin(PSERVER_DATA server, int app_id);

void streaming_interrupt();

void streaming_wait_for_stop();

void streaming_display_size(short width, short height);

bool streaming_running();

bool streaming_dispatch_event(SDL_Event ev);