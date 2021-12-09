#pragma once

#include "lvgl.h"
#include <SDL.h>

lv_disp_t *lv_app_display_init(SDL_Window *window);

void lv_app_display_deinit(lv_disp_t *disp);

void lv_app_display_resize(lv_disp_t *disp, int width, int height);

void lv_app_redraw_now(lv_disp_drv_t *disp_drv);

