#ifdef LV_LVGL_H_INCLUDE_SIMPLE

#include "lvgl.h"

#else
#include "../../lvgl.h"
#endif


#ifndef LV_FONT_EMPTY
#define LV_FONT_EMPTY 1
#endif

#if LV_FONT_EMPTY


/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
        /* U+0020 " " */
        0x0,
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
        {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
        {.bitmap_index = 0, .adv_w = 128, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 8},
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/



/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] = {
        {
                .range_start = 32, .range_length = 1, .glyph_id_start = 1,
                .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
        }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

/*Store all the custom data of the font*/
static lv_font_fmt_txt_glyph_cache_t cache;
static const lv_font_fmt_txt_dsc_t font_dsc = {
        .glyph_bitmap = glyph_bitmap,
        .glyph_dsc = glyph_dsc,
        .cmaps = cmaps,
        .kern_dsc = NULL,
        .kern_scale = 0,
        .cmap_num = 1,
        .bpp = 1,
        .kern_classes = 0,
        .bitmap_format = 0,
        .cache = &cache
};


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
const lv_font_t lv_font_empty = {
        .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
        .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
        .line_height = 9,          /*The maximum line height required by the font*/
        .base_line = 0,             /*Baseline measured from the bottom of the line*/
        .subpx = LV_FONT_SUBPX_NONE,
        .underline_position = 0,
        .underline_thickness = 0,
        .dsc = &font_dsc           /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
};

#endif /*#if LV_FONT_EMPTY*/