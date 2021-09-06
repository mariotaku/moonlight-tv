#pragma once

#include "backend/pcmanager.h"

#include <stdbool.h>

#include "libgamestream/client.h"
#include "lvgl/lv_sdl_img.h"

struct coverloader_t;

typedef struct coverloader_t coverloader_t;

coverloader_t *coverloader_new();

void coverloader_destroy(coverloader_t *);

void coverloader_display(coverloader_t *, PSERVER_LIST node, int id, lv_obj_t *target, lv_coord_t target_width,
                         lv_coord_t target_height);
