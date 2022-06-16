#include "lv_sdl_img.h"

#include <SDL_image.h>
#include "draw/sdl/lv_draw_sdl_utils.h"
#include "draw/sdl/lv_draw_sdl_texture_cache.h"

lv_res_t sdl_img_decoder_info(struct _lv_img_decoder_t *decoder, const void *src, lv_img_header_t *header);

lv_res_t sdl_img_decoder_open(struct _lv_img_decoder_t *decoder, struct _lv_img_decoder_dsc_t *dsc);

void sdl_img_decoder_close(struct _lv_img_decoder_t *decoder, struct _lv_img_decoder_dsc_t *dsc);

static bool is_sdl_img_src(const void *src);

static lv_draw_sdl_dec_dsc_userdata_t *lv_gpu_sdl_dec_dsc_userdata_new();

lv_img_decoder_t *lv_sdl_img_decoder_init(int flags) {
    IMG_Init(flags);
    lv_img_decoder_t *decoder = lv_img_decoder_create();
    lv_img_decoder_set_info_cb(decoder, sdl_img_decoder_info);
    lv_img_decoder_set_open_cb(decoder, sdl_img_decoder_open);
    lv_img_decoder_set_close_cb(decoder, sdl_img_decoder_close);
    return decoder;
}

lv_res_t sdl_img_decoder_info(struct _lv_img_decoder_t *decoder, const void *src, lv_img_header_t *header) {
    if (!is_sdl_img_src(src)) {
        return LV_RES_INV;
    }
    const lv_img_dsc_t *dsc = (lv_img_dsc_t *) src;
    header->w = dsc->header.w;
    header->h = dsc->header.h;
    header->cf = dsc->header.cf;
    return LV_RES_OK;
}

lv_res_t sdl_img_decoder_open(struct _lv_img_decoder_t *decoder, struct _lv_img_decoder_dsc_t *dsc) {
    if (!is_sdl_img_src(dsc->src)) {
        return LV_RES_INV;
    }
    const lv_sdl_img_data_t *sdl_src = (const lv_sdl_img_data_t *) ((lv_img_dsc_t *) dsc->src)->data;
    lv_draw_sdl_dec_dsc_userdata_t *userdata = lv_gpu_sdl_dec_dsc_userdata_new();
    switch (sdl_src->type) {
        case LV_SDL_IMG_TYPE_TEXTURE: {
            userdata->texture = sdl_src->data.texture;
            userdata->texture_managed = true;
            break;
        }
        case LV_SDL_IMG_TYPE_PATH: {
            lv_disp_drv_t *driver = _lv_refr_get_disp_refreshing()->driver;
            SDL_Renderer *renderer = ((lv_draw_sdl_drv_param_t *) driver->user_data)->renderer;
            SDL_Texture *texture = IMG_LoadTexture(renderer, sdl_src->data.path);
            userdata->texture = texture;
            break;
        }
        case LV_SDL_IMG_TYPE_CONST_PTR: {
            lv_disp_drv_t *driver = _lv_refr_get_disp_refreshing()->driver;
            SDL_Renderer *renderer = ((lv_draw_sdl_drv_param_t *) driver->user_data)->renderer;
            SDL_RWops *src = SDL_RWFromConstMem(sdl_src->data.constptr, (int) sdl_src->data_len);
            SDL_Texture *texture = IMG_LoadTexture_RW(renderer, src, 1);
            userdata->texture = texture;
            break;
        }
        default: {
            SDL_free(userdata);
            return LV_RES_INV;
        }
    }
    userdata->rect = sdl_src->rect;
    dsc->img_data = NULL;
    dsc->user_data = userdata;
    return LV_RES_OK;
}

void sdl_img_decoder_close(struct _lv_img_decoder_t *decoder, struct _lv_img_decoder_dsc_t *dsc) {
    if (!is_sdl_img_src(dsc->src)) {
        return;
    }
    lv_draw_sdl_dec_dsc_userdata_t *userdata = dsc->user_data;
    if (!userdata) {
        return;
    }
    if (!userdata->texture_referenced) {
        SDL_DestroyTexture(userdata->texture);
    }
    SDL_free(userdata);
}

static inline bool is_sdl_img_src(const void *src) {
    return lv_img_src_get_type(src) == LV_IMG_SRC_VARIABLE &&
           ((lv_img_dsc_t *) src)->data_size == sizeof(lv_sdl_img_data_t);
}

static lv_draw_sdl_dec_dsc_userdata_t *lv_gpu_sdl_dec_dsc_userdata_new() {
    lv_draw_sdl_dec_dsc_userdata_t *userdata = SDL_malloc(sizeof(lv_draw_sdl_dec_dsc_userdata_t));
    SDL_memset(userdata, 0, sizeof(lv_draw_sdl_dec_dsc_userdata_t));
    SDL_memcpy(userdata->head, LV_DRAW_SDL_DEC_DSC_TEXTURE_HEAD, 8);
    return userdata;
}
