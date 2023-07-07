#pragma once

#include "backend/pcmanager.h"

#include <stdbool.h>

#include "libgamestream/client.h"
#include "lv_sdl_img.h"

struct coverloader_t;

typedef struct app_t app_t;
typedef struct coverloader_t coverloader_t;

coverloader_t *coverloader_new(app_t *app);

void coverloader_unref(coverloader_t *loader);

void coverloader_display(coverloader_t *loader, const uuidstr_t *uuid, int id, lv_obj_t *target,
                         lv_coord_t target_width, lv_coord_t target_height);