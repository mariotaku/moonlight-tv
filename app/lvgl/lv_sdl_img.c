#include "lv_sdl_img.h"
#include "lv_disp_drv_app.h"

#include <mbedtls/base64.h>

#include <SDL_image.h>
#include "draw/sdl/lv_draw_sdl_utils.h"
#include "draw/sdl/lv_draw_sdl_texture_cache.h"

lv_res_t sdl_img_decoder_info(struct _lv_img_decoder_t *decoder, const void *src, lv_img_header_t *header);

lv_res_t sdl_img_decoder_open(struct _lv_img_decoder_t *decoder, struct _lv_img_decoder_dsc_t *dsc);

void sdl_img_decoder_close(struct _lv_img_decoder_t *decoder, struct _lv_img_decoder_dsc_t *dsc);

static bool is_sdl_img_src(const void *src);

static lv_draw_sdl_dec_dsc_userdata_t *lv_gpu_sdl_dec_dsc_userdata_new();

void lv_sdl_img_decoder_init(lv_img_decoder_t *decoder) {
    IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
    lv_img_decoder_set_info_cb(decoder, sdl_img_decoder_info);
    lv_img_decoder_set_open_cb(decoder, sdl_img_decoder_open);
    lv_img_decoder_set_close_cb(decoder, sdl_img_decoder_close);
}

void lv_sdl_img_src_stringify(const lv_sdl_img_src_t *src, char *str) {
    SDL_memcpy(str, LV_SDL_IMG_HEAD, 8);
    size_t olen;
    int ret = mbedtls_base64_encode((unsigned char *) &str[8], LV_SDL_IMG_LEN - 8, &olen,
                                    (const unsigned char *) src, sizeof(lv_sdl_img_src_t));
    LV_ASSERT(ret == 0);
    (void) ret;
}

void lv_sdl_img_src_parse(const char *str, lv_sdl_img_src_t *src) {
    size_t olen, slen = SDL_strlen(str) - 8;
    int ret = mbedtls_base64_decode((unsigned char *) src, sizeof(lv_sdl_img_src_t), &olen,
                                    (unsigned char *) &str[8], slen);
    LV_ASSERT(ret == 0);
    (void) ret;
}

lv_res_t sdl_img_decoder_info(struct _lv_img_decoder_t *decoder, const void *src, lv_img_header_t *header) {
    if (!is_sdl_img_src(src)) {
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
    if (!is_sdl_img_src(dsc->src)) {
        return LV_RES_INV;
    }
    lv_sdl_img_src_t sdl_src;
    lv_sdl_img_src_parse(dsc->src, &sdl_src);
    switch (sdl_src.type) {
        case LV_SDL_IMG_TYPE_TEXTURE: {
            dsc->img_data = NULL;
            lv_draw_sdl_dec_dsc_userdata_t *userdata = lv_gpu_sdl_dec_dsc_userdata_new();
            userdata->texture = sdl_src.data.texture;
            userdata->texture_managed = true;
            dsc->user_data = userdata;
            break;
        }
        case LV_SDL_IMG_TYPE_PATH: {
            SDL_Renderer *renderer = lv_draw_sdl_get_context()->renderer;
            SDL_Texture *texture = IMG_LoadTexture(renderer, sdl_src.data.path);
            dsc->img_data = NULL;
            lv_draw_sdl_dec_dsc_userdata_t *userdata = lv_gpu_sdl_dec_dsc_userdata_new();
            userdata->texture = texture;
            dsc->user_data = userdata;
            break;
        }
        case LV_SDL_IMG_TYPE_CONST_PTR: {
            SDL_Renderer *renderer = lv_draw_sdl_get_context()->renderer;
            SDL_RWops *src = SDL_RWFromConstMem(sdl_src.data.constptr, (int) sdl_src.data_len);
            SDL_Texture *texture = IMG_LoadTexture_RW(renderer, src, 1);
            dsc->img_data = NULL;
            lv_draw_sdl_dec_dsc_userdata_t *userdata = lv_gpu_sdl_dec_dsc_userdata_new();
            userdata->texture = texture;
            dsc->user_data = userdata;
            break;
        }
        default: {
            return LV_RES_INV;
        }
    }
    return LV_RES_OK;
}

void sdl_img_decoder_close(struct _lv_img_decoder_t *decoder, struct _lv_img_decoder_dsc_t *dsc) {
    if (!is_sdl_img_src(dsc->src)) {
        return;
    }
    lv_sdl_img_src_t sdl_src;
    lv_sdl_img_src_parse(dsc->src, &sdl_src);
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
    return ((char *) src)[0] == '!' && SDL_memcmp(src, LV_SDL_IMG_HEAD, 8) == 0;
}

static lv_draw_sdl_dec_dsc_userdata_t *lv_gpu_sdl_dec_dsc_userdata_new() {
    lv_draw_sdl_dec_dsc_userdata_t *userdata = SDL_malloc(sizeof(lv_draw_sdl_dec_dsc_userdata_t));
    SDL_memset(userdata, 0, sizeof(lv_draw_sdl_dec_dsc_userdata_t));
    SDL_memcpy(userdata->head, LV_DRAW_SDL_DEC_DSC_TEXTURE_HEAD, 8);
    return userdata;
}
