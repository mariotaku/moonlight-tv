#pragma once

#include "lvgl.h"
#include "lv_drv_sdl_key.h"

typedef struct app_t app_t;

typedef struct app_input_t {
} app_input_t;


void app_input_init(app_input_t *input, app_t *app);

void app_input_deinit(app_input_t *input);


void app_start_text_input(app_input_t *input,int x, int y, int w, int h);

void app_stop_text_input(app_input_t *input);

bool app_text_input_active(app_input_t *input);
