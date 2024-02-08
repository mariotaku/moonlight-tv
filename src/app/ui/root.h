#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <SDL.h>
#include <lvgl.h>

#include "ui/config.h"
#include "ui_input.h"
#include "util/font.h"

#include "util/navkey.h"

#include "lvgl/input/lv_drv_sdl_key.h"

typedef struct app_t app_t;
typedef struct app_fonts_t app_fonts_t;
typedef struct app_ui_t app_ui_t;
typedef struct app_launch_params_t app_launch_params_t;


struct app_ui_t {
    app_t *app;
    SDL_Window *window;
    SDL_Texture *video_texture;
    int width, height, dpi;
    lv_img_decoder_t *img_decoder;
    app_fonts_t fonts;
    lv_theme_t theme;

    // Can be created/destroyed multiple times
    app_ui_input_t input;
    lv_disp_t *disp;
    lv_obj_t *container;
    lv_fragment_manager_t *fm;
};

typedef struct {
    void *data1;
    void *data2;
} ui_userevent_t;

#define NAV_WIDTH_COLLAPSED 44
#define NAV_LOGO_SIZE 24

const lv_img_dsc_t *ui_logo_src();

void app_ui_init(app_ui_t *ui, app_t *app);

void app_ui_deinit(app_ui_t *ui);

void app_ui_open(app_ui_t *ui, const app_launch_params_t *params);

void app_ui_close(app_ui_t *ui);

bool app_ui_is_opened(const app_ui_t *ui);

bool ui_has_stream_renderer(app_ui_t *ui);

bool ui_render_background(app_ui_t *ui);

bool ui_dispatch_userevent(app_t *app, int which, void *data1, void *data2);

/**
 * @brief Check if GUI should consume input events, so it will not pass onto streaming
 */
bool ui_should_block_input();

void ui_display_size(app_ui_t *ui, int width, int height);


void ui_cb_destroy_fragment(lv_event_t *e);