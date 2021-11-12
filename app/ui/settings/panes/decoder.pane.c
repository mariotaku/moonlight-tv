#include "app.h"

#include "pref_obj.h"

#include "ui/settings/settings.controller.h"
#include "stream/platform.h"

#include "util/i18n.h"

typedef struct decoder_pane_t {
    lv_obj_controller_t base;
    settings_controller_t *parent;
    pref_dropdown_string_entry_t decoder_entries[DECODER_COUNT + 1];
    pref_dropdown_string_entry_t audio_entries[AUDIO_COUNT + 1];
} decoder_pane_t;

static lv_obj_t *create_obj(lv_obj_controller_t *self, lv_obj_t *parent);

static void pane_ctor(lv_obj_controller_t *self, void *args);

static void pref_mark_restart_cb(lv_event_t *e);

static void hdr_more_click_cb(lv_event_t *e);

const lv_obj_controller_class_t settings_pane_decoder_cls = {
        .constructor_cb = pane_ctor,
        .create_obj_cb = create_obj,
        .instance_size = sizeof(decoder_pane_t),
};

static void pane_ctor(lv_obj_controller_t *self, void *args) {
    decoder_pane_t *controller = (decoder_pane_t *) self;
    controller->parent = args;
    for (int type_idx = -1; type_idx < decoder_orders_len; type_idx++) {
        DECODER type = type_idx == -1 ? DECODER_AUTO : decoder_orders[type_idx];
        int index = type_idx + 1;
        pref_dropdown_string_entry_t *entry = &controller->decoder_entries[index];
        if (type == DECODER_AUTO) {
            entry->name = locstr("Automatic");
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
            entry->name = locstr("Automatic");
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
    lv_obj_t *decoder_label = pref_title_label(parent, locstr("Video decoder"));
    lv_label_set_text_fmt(decoder_label, locstr("Video decoder - %s selected"),
                          decoder_definitions[decoder_current].name);
    lv_obj_t *decoder_dropdown = pref_dropdown_string(parent, controller->decoder_entries, decoder_orders_len + 1,
                                                      &app_configuration->decoder);
    lv_obj_set_width(decoder_dropdown, LV_PCT(100));
    lv_obj_t *audio_label = pref_title_label(parent, locstr("Audio backend"));
    const char *audio_name = audio_current == AUDIO_DECODER ? locstr("Decoder provided")
                                                            : audio_definitions[audio_current].name;
    lv_label_set_text_fmt(audio_label, locstr("Audio backend - %s selected"), audio_name);
    lv_obj_t *audio_dropdown = pref_dropdown_string(parent, controller->audio_entries, audio_orders_len + 1,
                                                    &app_configuration->audio_backend);
    lv_obj_add_event_cb(decoder_dropdown, pref_mark_restart_cb, LV_EVENT_VALUE_CHANGED, controller);
    lv_obj_add_event_cb(audio_dropdown, pref_mark_restart_cb, LV_EVENT_VALUE_CHANGED, controller);
    lv_obj_set_width(audio_dropdown, LV_PCT(100));

    lv_obj_t *hdr_checkbox = pref_checkbox(parent, locstr("HDR (experimental)"), &app_configuration->stream.enableHdr,
                                           false);
    lv_obj_t *hdr_hint = pref_desc_label(parent, NULL);
    if (decoder_info.hdr == DECODER_HDR_NONE) {
        lv_obj_add_state(hdr_checkbox, LV_STATE_DISABLED);
        lv_label_set_text_fmt(hdr_hint, locstr("%s decoder doesn't support HDR."),
                              decoder_definitions[decoder_current].name);
    } else {
        lv_obj_clear_state(hdr_checkbox, LV_STATE_DISABLED);
        lv_label_set_text(hdr_hint, locstr("HDR is only supported on certain games and "
                                           "when connecting to supported monitor."));
    }
    lv_obj_t *hdr_more = pref_desc_label(parent, locstr("Learn more about HDR feature."));
    lv_obj_set_style_text_color(hdr_more, lv_theme_get_color_primary(hdr_more), 0);
    lv_obj_add_flag(hdr_more, LV_OBJ_FLAG_CLICKABLE);
    settings_controller_t *pcontroller = controller->parent;
    if (pcontroller) {
        lv_group_add_obj(pcontroller->mini ? pcontroller->group : pcontroller->detail_group, hdr_more);
    }
    lv_obj_add_event_cb(hdr_more, hdr_more_click_cb, LV_EVENT_CLICKED, NULL);

    return NULL;
}

static void pref_mark_restart_cb(lv_event_t *e) {
    decoder_pane_t *controller = (decoder_pane_t *) lv_event_get_user_data(e);
    settings_controller_t *parent = controller->parent;
    parent->needs_restart |= decoder_current != decoder_by_id(app_configuration->decoder);
    parent->needs_restart |= audio_current != audio_by_id(app_configuration->audio_backend);
}

static void hdr_more_click_cb(lv_event_t *e) {
    app_open_url("https://github.com/mariotaku/moonlight-tv/wiki/HDR-Support");
}