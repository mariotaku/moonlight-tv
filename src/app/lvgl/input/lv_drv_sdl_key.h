#pragma once

#include "lvgl.h"
#include <SDL_events.h>

typedef struct app_ui_input_t app_ui_input_t;

typedef struct lv_drv_sdl_key_t {
    lv_indev_drv_t base;
    uint32_t key, ev_key;
    lv_indev_state_t state;
    char text[SDL_TEXTINPUTEVENT_TEXT_SIZE];
    uint32_t text_len;
    uint8_t text_remain;
    uint32_t text_next;
    bool changed;
} lv_drv_sdl_key_t;

int lv_sdl_init_key_input(lv_drv_sdl_key_t *drv, app_ui_input_t *input);

void lv_sdl_key_input_release_key(lv_indev_t *indev);
