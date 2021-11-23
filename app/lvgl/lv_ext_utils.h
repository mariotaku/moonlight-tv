#pragma once

#include "lvgl.h"

lv_coord_t lv_measure_width(lv_obj_t *obj);

lv_coord_t lv_measure_height(lv_obj_t *obj);

void lv_area_center(const lv_area_t *area, lv_point_t *point);