#include "lv_sdl_img.h"

#include <SDL_image.h>

lv_res_t sdl_img_decoder_info(struct _lv_img_decoder_t *decoder, const void *src, lv_img_header_t *header);

lv_res_t sdl_img_decoder_open(struct _lv_img_decoder_t *decoder, struct _lv_img_decoder_dsc_t *dsc);

void sdl_img_decoder_close(struct _lv_img_decoder_t *decoder, struct _lv_img_decoder_dsc_t *dsc);

void lv_sdl_img_decoder_init(lv_img_decoder_t *decoder) {
    IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);

    decoder->info_cb = sdl_img_decoder_info;
    decoder->open_cb = sdl_img_decoder_open;
    decoder->close_cb = sdl_img_decoder_close;
}

lv_res_t sdl_img_decoder_info(struct _lv_img_decoder_t *decoder, const void *src, lv_img_header_t *header) {
    header->w = 300;
    header->h = 400;
    header->cf = LV_IMG_CF_TRUE_COLOR;
//    SDL_Surface *surface = IMG_Load("/home/pi/.moonlight-tv-covers/1088017781");
//    header->w = surface->w;
//    header->h = surface->h;
//    switch (surface->format->format) {
//        case SDL_PIXELFORMAT_ARGB8888:
//            header->cf = LV_IMG_CF_TRUE_COLOR_ALPHA;
//            break;
//        case SDL_PIXELFORMAT_RGB888:
//            header->cf = LV_IMG_CF_TRUE_COLOR;
//            break;
//        default:
//            header->cf = LV_IMG_CF_UNKNOWN;
//            break;
//    }
//    SDL_FreeSurface(surface);
    return LV_RES_OK;
}

lv_res_t sdl_img_decoder_open(struct _lv_img_decoder_t *decoder, struct _lv_img_decoder_dsc_t *dsc) {
    SDL_Surface *surface = IMG_Load("/home/pi/.moonlight-tv-covers/1088017781");
    if (surface->format->BytesPerPixel == 3) {
        SDL_Surface *converted = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_ARGB8888, 0);
        SDL_FreeSurface(surface);
        surface = converted;
    }
    dsc->img_data = surface->pixels;
    dsc->user_data = surface;
    return LV_RES_OK;
}

void sdl_img_decoder_close(struct _lv_img_decoder_t *decoder, struct _lv_img_decoder_dsc_t *dsc) {
    SDL_FreeSurface(dsc->user_data);
}