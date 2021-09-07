//
// Created by Mariotaku on 2021/08/30.
//

#include <malloc.h>
#include "basic.pane.h"

static lv_obj_t *on_create_view(lv_obj_controller_t *self, lv_obj_t *parent);

static void settings_pane_basic_ctor(lv_obj_controller_t *self, void *args);

const lv_obj_controller_class_t settings_pane_basic_cls = {
        .constructor_cb = settings_pane_basic_ctor,
        .destructor_cb = LV_OBJ_CONTROLLER_DTOR_DEF,
        .create_obj_cb = on_create_view,
        .instance_size = sizeof(lv_obj_controller_t),
};

static void settings_pane_basic_ctor(lv_obj_controller_t *self, void *args) {

}

static lv_obj_t *on_create_view(lv_obj_controller_t *self, lv_obj_t *parent) {
    lv_obj_t *view = lv_label_create(parent);
    lv_label_set_text(view, "Pane created");
    return view;
}