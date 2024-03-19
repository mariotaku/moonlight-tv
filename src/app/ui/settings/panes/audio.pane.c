#include "app.h"

#include "pref_obj.h"
#include "av_pane.h"

#include "ui/settings/settings.controller.h"

#include "util/i18n.h"
#include "ss4s.h"
#include "ss4s_modules.h"

typedef struct audio_pane_t {
    lv_fragment_t base;
    settings_controller_t *parent;

    lv_obj_t *conflict_hint;

    pref_dropdown_string_entry_t *adec_entries;
    int adec_entries_len;

    pref_dropdown_int_entry_t surround_entries[3];
    int surround_entries_len;
} audio_pane_t;

static lv_obj_t *create_obj(lv_fragment_t *self, lv_obj_t *container);

static void obj_created(lv_fragment_t *self, lv_obj_t *obj);

static void pane_ctor(lv_fragment_t *self, void *args);

static void pane_dtor(lv_fragment_t *self);

static void module_changed_cb(lv_event_t *e);

const lv_fragment_class_t settings_pane_audio_cls = {
        .constructor_cb = pane_ctor,
        .destructor_cb = pane_dtor,
        .create_obj_cb = create_obj,
        .obj_created_cb = obj_created,
        .instance_size = sizeof(audio_pane_t),
};

static void pane_ctor(lv_fragment_t *self, void *args) {
    audio_pane_t *pane = (audio_pane_t *) self;
    pane->parent = args;
    app_t *app = pane->parent->app;
    SS4S_AudioCapabilities audio_cap = app->ss4s.audio_cap;
    array_list_t modules = app->ss4s.modules;
    pane->adec_entries = calloc(modules.size + 1, sizeof(pref_dropdown_string_entry_t));

    set_decoder_entry(&pane->adec_entries[pane->adec_entries_len++], locstr("Auto"), "auto", true);
    for (int module_idx = 0; module_idx < modules.size; module_idx++) {
        const SS4S_ModuleInfo *info = array_list_get(&modules, module_idx);
        const char *group = SS4S_ModuleInfoGetGroup(info);
        if (info->has_audio && !contains_decoder_group(pane->adec_entries, pane->adec_entries_len, group)) {
            set_decoder_entry(&pane->adec_entries[pane->adec_entries_len++], info->name, group, false);
        }
    }
    unsigned int supported_ch = audio_cap.maxChannels;
    if (supported_ch == 0) {
        supported_ch = 2;
    }
#if FEATURE_EMBEDDED_SHELL
    if (!app_is_decoder_valid(app) && app_has_embedded(app)) {
        supported_ch = 8;
    }
#endif
    for (int i = 0; i < audio_config_len; i++) {
        audio_config_entry_t config = audio_configs[i];
        if (supported_ch < CHANNEL_COUNT_FROM_AUDIO_CONFIGURATION(config.configuration)) {
            continue;
        }
        struct pref_dropdown_int_entry_t *entry = &pane->surround_entries[pane->surround_entries_len];
        entry->name = locstr(config.name);
        entry->value = config.configuration;
        entry->fallback = config.configuration == AUDIO_CONFIGURATION_STEREO;
        pane->surround_entries_len++;
    }
}

static void pane_dtor(lv_fragment_t *self) {
    audio_pane_t *pane = (audio_pane_t *) self;
    free(pane->adec_entries);
}

static lv_obj_t *create_obj(lv_fragment_t *self, lv_obj_t *container) {
    audio_pane_t *controller = (audio_pane_t *) self;
    app_t *app = controller->parent->app;
    lv_obj_t *view = pref_pane_container(container);
    lv_obj_set_layout(view, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(view, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(view, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *audio_label = pref_title_label(view, locstr("Audio backend"));
    lv_label_set_text_fmt(audio_label, locstr("Audio backend - using %s"),
                          SS4S_ModuleInfoGetName(app->ss4s.selection.audio_module));
    lv_obj_t *adec_dropdown = pref_dropdown_string(view, controller->adec_entries, controller->adec_entries_len,
                                                   &app_configuration->audio_backend);
    lv_obj_set_width(adec_dropdown, LV_PCT(100));
#if FEATURE_EMBEDDED_SHELL
    if (!app_is_decoder_valid(app) && app_has_embedded(app)) {
        lv_obj_add_flag(audio_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(adec_dropdown, LV_OBJ_FLAG_HIDDEN);
    }
#endif

    lv_obj_t *conflict_hint = pref_desc_label(view, NULL, false);
    controller->conflict_hint = conflict_hint;

    pref_header(view, "Audio Settings");

    pref_title_label(view, locstr("Sound Channels (Experimental)"));

    lv_obj_t *ch_dropdown = pref_dropdown_int(view, controller->surround_entries, controller->surround_entries_len,
                                              &app_configuration->stream.audioConfiguration);
    lv_obj_set_width(ch_dropdown, LV_PCT(100));

    lv_obj_add_event_cb(adec_dropdown, module_changed_cb, LV_EVENT_VALUE_CHANGED, controller);

    return view;
}

static void obj_created(lv_fragment_t *self, lv_obj_t *obj) {
    audio_pane_t *fragment = (audio_pane_t *) self;
    update_conflict_hint(fragment->parent->app, fragment->conflict_hint);
}

static void module_changed_cb(lv_event_t *e) {
    audio_pane_t *fragment = (audio_pane_t *) lv_event_get_user_data(e);
    settings_controller_t *parent = fragment->parent;
    parent->needs_restart = true;
    update_conflict_hint(parent->app, fragment->conflict_hint);
}
