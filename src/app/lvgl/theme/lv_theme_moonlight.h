#pragma once

#include "lvgl.h"

typedef struct app_t app_t;

void lv_theme_moonlight_init(lv_theme_t *theme, const app_fonts_t *fonts, app_t *app);

void lv_theme_moonlight_deinit(lv_theme_t *theme);

const lv_font_t *lv_theme_moonlight_get_iconfont_large(lv_obj_t *obj);

const lv_font_t *lv_theme_moonlight_get_iconfont_normal(lv_obj_t *obj);

const lv_font_t *lv_theme_moonlight_get_iconfont_small(lv_obj_t *obj);

bool lv_theme_moonlight_is_oled(void);

lv_color_t lv_theme_moonlight_panel_color(void);

lv_color_t lv_theme_moonlight_sidebar_color(void);

lv_color_t lv_theme_moonlight_header_color(void);

lv_color_t lv_theme_moonlight_focus_color(lv_obj_t *obj);

lv_opa_t lv_theme_moonlight_focus_opa(void);
