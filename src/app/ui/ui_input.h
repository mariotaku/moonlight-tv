#pragma once

#include "lvgl.h"

#include "lvgl/input/lv_drv_sdl_key.h"

typedef struct app_ui_t app_ui_t;
typedef struct app_ui_input_t app_ui_input_t;
typedef enum app_ui_input_mode_t app_ui_input_mode_t;

typedef struct app_ui_input_lv_pair_t {
    lv_indev_drv_t drv;
    lv_indev_t *indev;
} app_ui_input_lv_pair_t;

enum app_ui_input_mode_t {
    UI_INPUT_MODE_POINTER_FLAG = 0x10,
    UI_INPUT_MODE_MOUSE = 0x11,
    UI_INPUT_MODE_REMOTE = 0x11,
    UI_INPUT_MODE_BUTTON_FLAG = 0x20,
    UI_INPUT_MODE_KEY = 0x21,
    UI_INPUT_MODE_GAMEPAD = 0x22,
};

struct app_ui_input_t {
    app_ui_t *ui;
    lv_group_t *app_group;
    lv_ll_t modal_groups;
    struct {
        lv_drv_sdl_key_t drv;
        lv_indev_t *indev;
    } key;
    app_ui_input_lv_pair_t pointer;
    app_ui_input_lv_pair_t wheel;
    app_ui_input_lv_pair_t button;
    app_ui_input_mode_t mode;
};

void app_ui_input_init(app_ui_input_t *input, app_ui_t *ui);

void app_ui_input_deinit(app_ui_input_t *input);

void app_input_set_group(app_ui_input_t *input, lv_group_t *group);

void app_input_push_modal_group(app_ui_input_t *input, lv_group_t *group);

void app_input_remove_modal_group(app_ui_input_t *input, lv_group_t *group);

lv_group_t *app_input_get_group(app_ui_input_t *input);

void app_input_set_button_points(app_ui_input_t *input, const lv_point_t *points);

bool ui_set_input_mode(app_ui_input_t *input, app_ui_input_mode_t mode);

app_ui_input_mode_t app_ui_get_input_mode(const app_ui_input_t *input);

void app_start_text_input(app_ui_input_t *input, int x, int y, int w, int h);

void app_stop_text_input(app_ui_input_t *input);

