#include "res.h"

#include "res/moonlight_png.h"
#include "res/fav_indicator_png.h"
#include "res/defcover_png.h"
#include "res/material_icons_regular_ttf.h"

const lv_sdl_img_data_t lv_sdl_img_data_logo_96 = {
        .type = LV_SDL_IMG_TYPE_CONST_PTR,
        .data.constptr = res_moonlight_png_data,
        .data_len = res_moonlight_png_size,
};

const lv_sdl_img_data_t lv_sdl_img_data_fav_indicator = {
        .type = LV_SDL_IMG_TYPE_CONST_PTR,
        .data.constptr = res_fav_indicator_png_data,
        .data_len = res_fav_indicator_png_size,
};

const lv_sdl_img_data_t lv_sdl_img_data_defcover = {
        .type = LV_SDL_IMG_TYPE_CONST_PTR,
        .data.constptr = res_defcover_png_data,
        .data_len = res_defcover_png_size,
};

const unsigned char *res_mat_iconfont_data = res_material_icons_regular_ttf_data;
const unsigned int res_mat_iconfont_size = res_material_icons_regular_ttf_size;