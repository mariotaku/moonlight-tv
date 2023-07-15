#include "lv_ext_utils.h"

void lv_area_center(const lv_area_t *area, lv_point_t *point) {
    point->x = area->x1 + lv_area_get_width(area) / 2;
    point->y = area->y1 + lv_area_get_height(area) / 2;
}