#include "lv_sdl_img.h"

#include <SDL_image.h>

static char img_fmt[64] = "";

lv_res_t sdl_img_decoder_info(struct _lv_img_decoder_t *decoder, const void *src, lv_img_header_t *header);

lv_res_t sdl_img_decoder_open(struct _lv_img_decoder_t *decoder, struct _lv_img_decoder_dsc_t *dsc);

void sdl_img_decoder_close(struct _lv_img_decoder_t *decoder, struct _lv_img_decoder_dsc_t *dsc);

static void img_fmt_init();

void lv_sdl_img_decoder_init(lv_img_decoder_t *decoder) {
    IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
    lv_img_decoder_set_info_cb(decoder, sdl_img_decoder_info);
    lv_img_decoder_set_open_cb(decoder, sdl_img_decoder_open);
    lv_img_decoder_set_close_cb(decoder, sdl_img_decoder_close);
}

void lv_sdl_img_src_stringify(const lv_sdl_img_src_t *src, char *str) {
    img_fmt_init();
    SDL_memcpy(str, LV_SDL_IMG_HEAD, 8);
    SDL_snprintf(&str[8], LV_SDL_IMG_LEN - 8, img_fmt, src->w, src->h, src->cf, src->type, src->data.pointer);
}

void lv_sdl_img_src_parse(const char *str, lv_sdl_img_src_t *src) {
    img_fmt_init();
    SDL_sscanf(&str[8], img_fmt, &src->w, &src->h, &src->cf, &src->type, &src->data.pointer);
}

lv_res_t sdl_img_decoder_info(struct _lv_img_decoder_t *decoder, const void *src, lv_img_header_t *header) {
    if (SDL_memcmp(src, LV_SDL_IMG_HEAD, 8) != 0) {
        return LV_RES_INV;
    }
    lv_sdl_img_src_t sdl_src;
    lv_sdl_img_src_parse(src, &sdl_src);
    header->w = sdl_src.w;
    header->h = sdl_src.h;
    header->cf = sdl_src.cf;
    return LV_RES_OK;
}

lv_res_t sdl_img_decoder_open(struct _lv_img_decoder_t *decoder, struct _lv_img_decoder_dsc_t *dsc) {
    if (SDL_memcmp(dsc->src, LV_SDL_IMG_HEAD, 8) != 0) {
        return LV_RES_INV;
    }
    lv_sdl_img_src_t sdl_src;
    lv_sdl_img_src_parse(dsc->src, &sdl_src);
    SDL_Surface *surface;
    switch (sdl_src.type) {
        case LV_SDL_IMG_TYPE_SURFACE: {
            surface = sdl_src.data.surface;
            break;
        }
        case LV_SDL_IMG_TYPE_PATH: {
            surface = IMG_Load(sdl_src.data.path);
            if (surface->format->BytesPerPixel == 3) {
                SDL_Surface *converted = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_ARGB8888, 0);
                SDL_FreeSurface(surface);
                surface = converted;
            }
            break;
        }
        default: {
            return LV_RES_INV;
        }
    }
    dsc->img_data = surface->pixels;
    dsc->user_data = surface;
    return LV_RES_OK;
}

void sdl_img_decoder_close(struct _lv_img_decoder_t *decoder, struct _lv_img_decoder_dsc_t *dsc) {
    if (SDL_memcmp(dsc->src, LV_SDL_IMG_HEAD, 8) != 0) {
        return;
    }
    lv_sdl_img_src_t sdl_src;
    lv_sdl_img_src_parse(dsc->src, &sdl_src);
    switch (sdl_src.type) {
        case LV_SDL_IMG_TYPE_PATH: {
            SDL_FreeSurface(dsc->user_data);
            break;
        }
        default: {
            // Ignore
            break;
        }
    }
}


static void img_fmt_init() {
    if (img_fmt[0]) return;
    SDL_snprintf(img_fmt, 64, "%%0%dx%%0%dx%%0%dx%%0%dx%%0%dx", sizeof(lv_coord_t) * 2, sizeof(lv_coord_t) * 2,
                 sizeof(lv_img_cf_t) * 2, sizeof(lv_sdl_img_src_type_t) * 2, sizeof(void *) * 2);
}