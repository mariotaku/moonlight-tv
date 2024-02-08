#pragma once

#include "lvgl.h"
#include <SDL.h>

lv_disp_drv_t *lv_app_disp_drv_create(SDL_Window *window, int dpi, void *user_data);

void lv_app_disp_drv_deinit(lv_disp_drv_t *driver);

void lv_app_display_resize(lv_disp_t *disp, int width, int height);

void lv_app_redraw_now(lv_disp_drv_t *disp_drv);

