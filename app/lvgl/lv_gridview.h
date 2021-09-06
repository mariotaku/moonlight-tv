#pragma once

#include "lvgl.h"

extern const lv_obj_class_t lv_gridview_class;

typedef struct lv_grid_adapter_t {
    int (*item_count)(lv_obj_t *, void *data);

    lv_obj_t *(*create_view)(lv_obj_t *parent);

    void (*bind_view)(lv_obj_t *, lv_obj_t *item_view, void *data, int position);

    int (*item_id)(lv_obj_t *, void *data, int position);
} lv_grid_adapter_t;

lv_obj_t *lv_gridview_create(lv_obj_t *parent);

void lv_gridview_set_config(lv_obj_t *obj, int col_count, lv_coord_t row_height);

void lv_grid_set_adapter(lv_obj_t *obj, const lv_grid_adapter_t *adapter);

void lv_grid_set_data(lv_obj_t *obj, void *data);

void lv_gridview_focus(lv_obj_t *obj, int index);

int lv_gridview_get_focused_index(lv_obj_t *obj);