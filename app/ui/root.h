#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <SDL.h>
#include <lvgl.h>

#include "ui/config.h"

#include "util/navkey.h"

#include "input/lv_drv_sdl_key.h"

typedef struct app_t app_t;
typedef struct app_fonts_t app_fonts_t;

typedef struct app_ui_input_lv_pair_t {
    lv_indev_drv_t drv;
    lv_indev_t *indev;
} app_ui_input_lv_pair_t;

typedef struct app_ui_input_t {
    lv_group_t *app_group;
    lv_ll_t modal_groups;
    struct {
        lv_drv_sdl_key_t drv;
        lv_indev_t *indev;
    } key;
    app_ui_input_lv_pair_t pointer;
    app_ui_input_lv_pair_t wheel;
    app_ui_input_lv_pair_t button;
} app_ui_input_t;

typedef struct app_ui_t {
    app_t *app;
    SDL_Window *window;
    lv_img_decoder_t *img_decoder;
    lv_theme_t theme;
    app_fonts_t *fonts;
    lv_disp_t *disp;
    app_ui_input_t input;
} app_ui_t;

enum UI_INPUT_MODE {
    UI_INPUT_MODE_POINTER_FLAG = 0x10,
    UI_INPUT_MODE_MOUSE = 0x11,
    UI_INPUT_MODE_REMOTE = 0x11,
    UI_INPUT_MODE_BUTTON_FLAG = 0x20,
    UI_INPUT_MODE_KEY = 0x21,
    UI_INPUT_MODE_GAMEPAD = 0x22,
};

typedef struct {
    void *data1;
    void *data2;
} ui_userevent_t;

#define NAV_WIDTH_COLLAPSED 44
#define NAV_LOGO_SIZE 24

extern short ui_display_width, ui_display_height;
extern enum UI_INPUT_MODE ui_input_mode;

const lv_img_dsc_t *ui_logo_src();

void app_ui_init(app_ui_t *ui, app_t *app);

void app_ui_deinit(app_ui_t *ui);

void app_ui_open(app_ui_t *ui);

void app_ui_close(app_ui_t *ui);

void app_ui_input_init(app_ui_input_t *input, app_ui_t*ui);

void app_ui_input_deinit(app_ui_input_t *input);

void app_input_set_group(app_ui_input_t *input, lv_group_t *group);

void app_input_push_modal_group(app_ui_input_t *input, lv_group_t *group);

void app_input_remove_modal_group(app_ui_input_t *input, lv_group_t *group);

lv_group_t *app_input_get_group(app_ui_input_t *input);

void app_input_set_button_points(app_ui_input_t *input, const lv_point_t *points);

bool ui_has_stream_renderer();

bool ui_render_background();

bool ui_dispatch_userevent(app_t *app, int which, void *data1, void *data2);

/**
 * @brief Check if GUI should consume input events, so it will not pass onto streaming
 */
bool ui_should_block_input();

void ui_display_size(short width, short height);

bool ui_set_input_mode(enum UI_INPUT_MODE mode);

void ui_cb_destroy_fragment(lv_event_t *e);