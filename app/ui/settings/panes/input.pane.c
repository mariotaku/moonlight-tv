#include "app.h"

#include "pref_obj.h"

#include "util/i18n.h"

static lv_obj_t *create_obj(lv_fragment_t *self, lv_obj_t *view);

static void pane_ctor(lv_fragment_t *self, void *args);

const lv_fragment_class_t settings_pane_input_cls = {
        .constructor_cb = pane_ctor,
        .create_obj_cb = create_obj,
        .instance_size = sizeof(lv_fragment_t),
};

static void pane_ctor(lv_fragment_t *self, void *args) {

}

static lv_obj_t *create_obj(lv_fragment_t *self, lv_obj_t *container) {
    lv_obj_t *view = pref_pane_container(container);
    lv_obj_set_layout(view, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(view, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(view, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    pref_checkbox(view, locstr("View-only mode"), &app_configuration->viewonly, false);
    pref_desc_label(view, locstr("Don't send mouse, keyboard or gamepad input to host computer."), false);

    pref_checkbox(view, locstr("Absolute mouse mode"), &app_configuration->absmouse, false);
    pref_desc_label(view, locstr("Better for remote desktop. For some games, mouse will not work properly."), false);

    pref_checkbox(view, locstr("Virtual mouse"), &app_configuration->virtual_mouse, false);
    pref_desc_label(view, locstr("Press LB + Right stick to move mouse cursor with sticks. "
                                 "LT/RT for left/right mouse buttons."), false);

    pref_checkbox(view, locstr("Swap ABXY buttons"), &app_configuration->swap_abxy, false);
    pref_desc_label(view, locstr("Swap A/B and X/Y gamepad buttons. Useful when you prefer Nintendo-like layouts."),
                    false);
    return view;
}