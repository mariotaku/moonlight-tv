#include "lv_ext_utils.h"

#include <assert.h>

lv_coord_t lv_measure_width(lv_obj_t *obj) {
    lv_coord_t width;
    int percent = 100;
    do {
        width = lv_obj_get_style_width(obj, 0);
        if (LV_COORD_IS_PCT(width)) {
            percent = percent * LV_COORD_GET_PCT(width) / 100;
            obj = lv_obj_get_parent(obj);
        } else {
            assert(LV_COORD_IS_PX(width));
            return width * percent / 100;
        }
    } while (obj && !LV_COORD_IS_PX(width));
    return 0;
}

lv_coord_t lv_measure_height(lv_obj_t *obj) {
    lv_coord_t height;
    int percent = 100;
    do {
        height = lv_obj_get_style_height(obj, 0);
        if (LV_COORD_IS_PCT(height)) {
            percent = percent * LV_COORD_GET_PCT(height) / 100;
            obj = lv_obj_get_parent(obj);
        } else {
            assert(LV_COORD_IS_PX(height));
            return height * percent / 100;
        }
    } while (obj && !LV_COORD_IS_PX(height));
    return 0;
}