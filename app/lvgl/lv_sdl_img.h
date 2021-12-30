#pragma once

#include "lvgl.h"

#include <SDL.h>

#define LV_SDL_IMG_HEAD "!SDLIMG:"

typedef enum {
    LV_SDL_IMG_TYPE_PATH,
    LV_SDL_IMG_TYPE_CONST_PTR,
    LV_SDL_IMG_TYPE_TEXTURE,
} lv_sdl_img_src_type_t;

typedef struct {
    lv_coord_t w, h;
    lv_img_cf_t cf: 8;
    lv_sdl_img_src_type_t type: 8;
    union {
        void *pointer;
        const void *constptr;
        const char *path;
        SDL_Surface *surface;
        SDL_Texture *texture;
    } data;
    unsigned int data_len;
    SDL_Rect rect;
} lv_sdl_img_src_t;

#define LV_SDL_IMG_LEN (8 + sizeof(lv_sdl_img_src_t) * 4 / 3 + 4)

void lv_sdl_img_decoder_init(lv_img_decoder_t *decoder);

void lv_sdl_img_src_stringify(const lv_sdl_img_src_t *src, char *str);

void lv_sdl_img_src_parse(const char *str, lv_sdl_img_src_t *src);