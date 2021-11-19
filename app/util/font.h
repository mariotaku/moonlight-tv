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

extern app_fontset_t app_iconfonts;

bool app_font_init(lv_theme_t *theme);
