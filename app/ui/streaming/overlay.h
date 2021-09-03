#pragma once

#include <stdbool.h>
#include <string.h>

#include "lvgl.h"

#include "ui/config.h"


#include "backend/pcmanager.h"
#include "stream/session.h"

#include "util/navkey.h"

typedef struct {
    const SERVER_DATA *server;
    const APP_DLIST *app;
} STREAMING_SCENE_ARGS;

extern bool stream_overlay_showing;

void streaming_overlay_init();

bool streaming_overlay_should_block_input();

bool streaming_overlay_hide();

bool streaming_overlay_show();
