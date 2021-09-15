//
// Created by Mariotaku on 2021/08/30.
//

#include <malloc.h>
#include <lvgl.h>
#include <app.h>
#include <stream/platform.h>
#include "lvgl/ext/lv_obj_controller.h"
#include "pref_obj.h"

typedef struct decoder_pane_t {
    pref_dropdown_string_entry_t decoder_entries[DECODER_COUNT];
    pref_dropdown_string_entry_t audio_entries[AUDIO_COUNT];
} decoder_pane_t;

static lv_obj_t *create_obj(lv_obj_controller_t *self, lv_obj_t *parent);

static void pane_ctor(lv_obj_controller_t *self, void *args);

const lv_obj_controller_class_t settings_pane_decoder_cls = {
        .constructor_cb = pane_ctor,
        .create_obj_cb = create_obj,
        .instance_size = sizeof(decoder_pane_t),
};

static void pane_ctor(lv_obj_controller_t *self, void *args) {
    decoder_pane_t *controller = (decoder_pane_t *) self;
    for (int i = 0; i < DECODER_COUNT; i++) {
        if (i == DECODER_NONE) {
            controller->decoder_entries[i].name = "Automatic";
            controller->decoder_entries[i].value = "auto";
            controller->decoder_entries[i].fallback = true;
        } else {
            controller->decoder_entries[i].name = decoder_definitions[i].name;
            controller->decoder_entries[i].value = decoder_definitions[i].id;
            controller->decoder_entries[i].fallback = false;
        }
    }
    for (int i = 0; i < AUDIO_COUNT; i++) {
        controller->audio_entries[i].name = audio_definitions[i].name;
        controller->audio_entries[i].value = audio_definitions[i].id;
        controller->audio_entries[i].fallback = false;
    }
}

static lv_obj_t *create_obj(lv_obj_controller_t *self, lv_obj_t *parent) {
    decoder_pane_t *controller = (decoder_pane_t *) self;
    lv_obj_set_layout(parent, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    pref_title_label(parent, "Video decoder");
    lv_obj_t *decoder_dropdown = pref_dropdown_string(parent, controller->decoder_entries, DECODER_COUNT,
                                                      &app_configuration->decoder);
    lv_obj_set_width(decoder_dropdown, LV_PCT(100));
    pref_title_label(parent, "Audio backend");
    lv_obj_t *audio_dropdown = pref_dropdown_string(parent, controller->audio_entries, AUDIO_COUNT,
                                                    &app_configuration->audio_backend);
    lv_obj_set_width(audio_dropdown, LV_PCT(100));
    return NULL;
}