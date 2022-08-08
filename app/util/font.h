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
    struct app_fontset_t *fallback;
} app_fontset_t;

typedef struct app_fonts_t {
    app_fontset_t fonts;
    app_fontset_t icons;
} app_fonts_t;

extern app_fontset_t app_iconfonts;

app_fonts_t *app_font_init(lv_theme_t *theme);

void app_font_deinit(app_fonts_t *fonts);
