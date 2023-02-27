#pragma once

#include "lvgl.h"
#include "lv_drv_sdl_key.h"

typedef struct app_t app_t;

typedef struct app_input_lv_pair_t {
    lv_indev_drv_t drv;
    lv_indev_t *indev;
} app_input_lv_pair_t;

typedef struct app_input_t {
    struct {
        lv_drv_sdl_key_t drv;
        lv_indev_t *indev;
    } key;
    app_input_lv_pair_t pointer;
    app_input_lv_pair_t wheel;
    app_input_lv_pair_t button;
} app_input_t;


void app_input_init(app_input_t *input, app_t *app);

void app_input_deinit(app_input_t *input);

void app_input_set_group(app_input_t *input, lv_group_t *group);

void app_input_push_modal_group(app_input_t *input, lv_group_t *group);

void app_input_remove_modal_group(app_input_t *input, lv_group_t *group);

lv_group_t *app_input_get_group(app_input_t *input);

void app_input_set_button_points(app_input_t *input, const lv_point_t *points);

void app_start_text_input(app_input_t *input,int x, int y, int w, int h);

void app_stop_text_input(app_input_t *input);

bool app_text_input_active(app_input_t *input);
