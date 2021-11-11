#pragma once

#include "lvgl.h"

#include <fontconfig/fontconfig.h>

typedef struct app_fontset_t {
    int small_size;
    int normal_size;
    int large_size;
    lv_font_t *small;
    lv_font_t *normal;
    lv_font_t *large;
} app_fontset_t;

bool app_fontset_load(app_fontset_t *set, FcPattern *font);