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
    pref_dropdown_string_entry_t decoder_entries[DECODER_COUNT + 1];
    pref_dropdown_string_entry_t audio_entries[AUDIO_COUNT + 1];
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
    for (int type_idx = -1; type_idx < decoder_orders_len; type_idx++) {
        DECODER type = type_idx == -1 ? DECODER_AUTO : decoder_orders[type_idx];
        int index = type_idx + 1;
        pref_dropdown_string_entry_t *entry = &controller->decoder_entries[index];
        if (type == DECODER_AUTO) {
            entry->name = "Automatic";
            entry->value = "auto";
            entry->fallback = true;
        } else {
            MODULE_DEFINITION def = decoder_definitions[type];
            entry->name = def.name;
            entry->value = def.id;
            entry->fallback = false;
        }
    }
    for (int type_idx = -1; type_idx < audio_orders_len; type_idx++) {
        AUDIO type = type_idx == -1 ? AUDIO_AUTO : audio_orders[type_idx];
        int index = type_idx + 1;
        pref_dropdown_string_entry_t *entry = &controller->audio_entries[index];
        if (type == AUDIO_AUTO) {
            entry->name = "Automatic";
            entry->value = "auto";
            entry->fallback = true;
        } else {
            MODULE_DEFINITION def = audio_definitions[type];
            entry->name = def.name;
            entry->value = def.id;
            entry->fallback = false;
        }
    }
}

static lv_obj_t *create_obj(lv_obj_controller_t *self, lv_obj_t *parent) {
    decoder_pane_t *controller = (decoder_pane_t *) self;
    lv_obj_set_layout(parent, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    pref_title_label(parent, "Video decoder");
    lv_obj_t *decoder_dropdown = pref_dropdown_string(parent, controller->decoder_entries, decoder_orders_len + 1,
                                                      &app_configuration->decoder);
    lv_obj_set_width(decoder_dropdown, LV_PCT(100));
    pref_title_label(parent, "Audio backend");
    lv_obj_t *audio_dropdown = pref_dropdown_string(parent, controller->audio_entries, audio_orders_len + 1,
                                                    &app_configuration->audio_backend);
    lv_obj_set_width(audio_dropdown, LV_PCT(100));
    return NULL;
}