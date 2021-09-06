//
// Created by Mariotaku on 2021/08/30.
//

#include <malloc.h>
#include "basic.pane.h"

static lv_obj_t *on_create_view(lv_obj_controller_t *self, lv_obj_t *parent);

lv_obj_controller_t *settings_pane_basic(void *args) {
    lv_obj_controller_t *controller = malloc(sizeof(lv_obj_controller_t));
    memset(controller, 0, sizeof(lv_obj_controller_t));
    controller->create_view = on_create_view;
    controller->destroy_controller = ui_view_controller_free;
    return controller;
}

static lv_obj_t *on_create_view(lv_obj_controller_t *self, lv_obj_t *parent) {
    lv_obj_t *view = lv_label_create(parent);
    lv_label_set_text(view, "Pane created");
    return view;
}