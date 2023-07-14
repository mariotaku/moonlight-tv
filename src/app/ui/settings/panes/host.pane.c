#include "app.h"

#include "pref_obj.h"

#include "util/i18n.h"

static lv_obj_t *create_obj(lv_fragment_t *self, lv_obj_t *view);

static void pane_ctor(lv_fragment_t *self, void *args);

const lv_fragment_class_t settings_pane_host_cls = {
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
    pref_checkbox(view, locstr("Optimize game settings for streaming"), &app_configuration->sops, false);
    pref_desc_label(view, locstr("Change in-game settings to optimize for streaming. Resolution will be "
                                 "limited to 720p, 1080p or 4K. Framerate will be also limited to 30/60 FPS."),
                    false);
    pref_checkbox(view, locstr("Mute computer while streaming"), &app_configuration->localaudio, true);
    return view;
}