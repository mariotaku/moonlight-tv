#pragma once

#include "lvgl.h"

#include <SDL.h>

typedef enum {
    LV_SDL_IMG_TYPE_PATH,
    LV_SDL_IMG_TYPE_CONST_PTR,
    LV_SDL_IMG_TYPE_TEXTURE,
} lv_sdl_img_data_type_t;

typedef struct {
    lv_sdl_img_data_type_t type: 8;
    union {
        void *pointer;
        const void *constptr;
        const char *path;
        SDL_Surface *surface;
        SDL_Texture *texture;
    } data;
    unsigned int data_len;
    SDL_Rect rect;
} lv_sdl_img_data_t;

lv_img_decoder_t *lv_sdl_img_decoder_init(int flags);
