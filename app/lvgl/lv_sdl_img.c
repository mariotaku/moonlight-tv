#include "lv_sdl_img.h"

#include <SDL_image.h>
#include <gpu/sdl/lv_gpu_sdl_texture_cache.h>

static char img_fmt[LV_SDL_IMG_LEN] = "";

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
    SDL_snprintf(&str[8], LV_SDL_IMG_LEN - 8, img_fmt, src->w, src->h, src->cf, src->type,
                 src->data.pointer, src->data_len);
}

void lv_sdl_img_src_parse(const char *str, lv_sdl_img_src_t *src) {
    img_fmt_init();
    SDL_sscanf(&str[8], img_fmt, &src->w, &src->h, &src->cf, &src->type, &src->data.pointer, &src->data_len);
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
    switch (sdl_src.type) {
        case LV_SDL_IMG_TYPE_SURFACE: {
            dsc->img_data = sdl_src.data.surface->pixels;
            dsc->user_data = NULL;
            break;
        }
        case LV_SDL_IMG_TYPE_TEXTURE: {
            dsc->img_data = NULL;
            lv_gpu_sdl_dec_dsc_userdata_t *userdata = SDL_malloc(sizeof(lv_gpu_sdl_dec_dsc_userdata_t));
            SDL_memcpy(userdata->head, LV_GPU_SDL_DEC_DSC_TEXTURE_HEAD, 8);
            userdata->texture = sdl_src.data.texture;
            dsc->user_data = userdata;
            break;
        }
        case LV_SDL_IMG_TYPE_PATH: {
            SDL_Surface *surface = IMG_Load(sdl_src.data.path);
            if (surface->format->BytesPerPixel == 3) {
                SDL_Surface *converted = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_ARGB8888, 0);
                SDL_FreeSurface(surface);
                surface = converted;
            }
            dsc->img_data = surface->pixels;
            dsc->user_data = surface;
            break;
        }
        case LV_SDL_IMG_TYPE_CONST_PTR: {
            SDL_Surface *surface = IMG_Load_RW(SDL_RWFromConstMem(sdl_src.data.pointer, sdl_src.data_len), 1);
            if (surface->format->BytesPerPixel == 3) {
                SDL_Surface *converted = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_ARGB8888, 0);
                SDL_FreeSurface(surface);
                surface = converted;
            }
            dsc->img_data = surface->pixels;
            dsc->user_data = surface;
            break;
        }
        default: {
            return LV_RES_INV;
        }
    }
    return LV_RES_OK;
}

void sdl_img_decoder_close(struct _lv_img_decoder_t *decoder, struct _lv_img_decoder_dsc_t *dsc) {
    if (SDL_memcmp(dsc->src, LV_SDL_IMG_HEAD, 8) != 0) {
        return;
    }
    lv_sdl_img_src_t sdl_src;
    lv_sdl_img_src_parse(dsc->src, &sdl_src);
    switch (sdl_src.type) {
        case LV_SDL_IMG_TYPE_PATH:
        case LV_SDL_IMG_TYPE_CONST_PTR: {
            SDL_FreeSurface(dsc->user_data);
            break;
        }
        case LV_SDL_IMG_TYPE_TEXTURE: {
            SDL_free(dsc->user_data);
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
    SDL_snprintf(img_fmt, LV_SDL_IMG_LEN, "%%0%dx%%0%dx%%0%dx%%0%dx%%0%dx%%0%dx", sizeof(lv_coord_t) * 2,
                 sizeof(lv_coord_t) * 2, sizeof(lv_img_cf_t) * 2, sizeof(lv_sdl_img_src_type_t) * 2, sizeof(void *) * 2,
                 sizeof(int) * 2);
}