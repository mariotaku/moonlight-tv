#pragma once

#include "pcmanager.h"

#include <stdbool.h>

#include "libgamestream/client.h"
#include "lvgl/lv_sdl_img.h"

#define MAIN_THREAD
#define WORKER_THREAD
#define THREAD_SAFE

MAIN_THREAD void coverloader_init();

MAIN_THREAD void coverloader_destroy();

MAIN_THREAD void coverloader_display(PSERVER_LIST node, int id, lv_obj_t *target, lv_coord_t target_size);

#ifndef _COVERLOADER_IMPL
#undef MAIN_THREAD
#undef WORKER_THREAD
#undef THREAD_SAFE
#endif