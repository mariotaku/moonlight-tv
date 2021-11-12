#include "app.h"

#include "pref_obj.h"

#include "util/i18n.h"

static lv_obj_t *create_obj(lv_obj_controller_t *self, lv_obj_t *parent);

static void pane_ctor(lv_obj_controller_t *self, void *args);

const lv_obj_controller_class_t settings_pane_host_cls = {
        .constructor_cb = pane_ctor,
        .create_obj_cb = create_obj,
        .instance_size = sizeof(lv_obj_controller_t),
};

static void pane_ctor(lv_obj_controller_t *self, void *args) {

}

static lv_obj_t *create_obj(lv_obj_controller_t *self, lv_obj_t *parent) {
    lv_obj_set_layout(parent, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    pref_checkbox(parent, locstr("Optimize game settings for streaming"), &app_configuration->sops, false);
    pref_desc_label(parent, locstr("This will change in-game settings to optimize it for streaming, "
                                   "and limit resolution to 720p, 1080p or 4K. Framerate will be also limited "
                                   "to 30/60 FPS. Turn off if you want to try unsupported resolution and FPS."));
    pref_checkbox(parent, locstr("Mute host PC while streaming"), &app_configuration->localaudio, true);
    return NULL;
}