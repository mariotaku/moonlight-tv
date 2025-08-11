#include "app.h"
#include "config.h"

#include "pref_obj.h"
#include "pref_fps.h"
#include "pref_res.h"
#include "ui/settings/settings.controller.h"

#include "util/i18n.h"

typedef struct {
    lv_fragment_t base;
    settings_controller_t *parent;

    lv_obj_t *res_warning;
    lv_obj_t *bitrate_label;
    lv_obj_t *bitrate_slider;
    lv_obj_t *bitrate_warning;

    pref_dropdown_string_entry_t *lang_entries;
    int lang_entries_len;
} basic_pane_t;

static void pane_ctor(lv_fragment_t *self, void *args);

static void pane_dtor(lv_fragment_t *self);

static lv_obj_t *create_obj(lv_fragment_t *self, lv_obj_t *container);

static void on_bitrate_changed(lv_event_t *e);

static void on_res_fps_updated(lv_event_t *e);

static void on_fullscreen_updated(lv_event_t *e);

static void update_bitrate_label(basic_pane_t *pane);

static void init_locale_entries(basic_pane_t *pane);

static void pref_mark_restart_cb(lv_event_t *e);

static void update_bitrate_hint(basic_pane_t *pane);

const lv_fragment_class_t settings_pane_basic_cls = {
    .constructor_cb = pane_ctor,
    .destructor_cb = pane_dtor,
    .create_obj_cb = create_obj,
    .instance_size = sizeof(basic_pane_t),
};
#define BITRATE_STEP 1000

static void pane_ctor(lv_fragment_t *self, void *args) {
    basic_pane_t *pane = (basic_pane_t *) self;
    pane->parent = args;
#ifdef FEATURE_I18N_LANGUAGE_SETTINGS
    init_locale_entries(pane);
#endif
}

static void pane_dtor(lv_fragment_t *self) {
    basic_pane_t *pane = (basic_pane_t *) self;
#ifdef FEATURE_I18N_LANGUAGE_SETTINGS
    lv_mem_free(pane->lang_entries);
#endif
}

static lv_obj_t *create_obj(lv_fragment_t *self, lv_obj_t *container) {
    basic_pane_t *pane = (basic_pane_t *) self;
    settings_controller_t *parent = pane->parent;
    app_t *app = parent->app;
    lv_obj_t *view = pref_pane_container(container);
    lv_obj_set_layout(view, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(view, LV_FLEX_FLOW_ROW_WRAP);
    pref_title_label(view, locstr("Resolution and FPS"));


    int max_width = (int) app->ss4s.video_cap.maxWidth, max_height = (int) app->ss4s.video_cap.maxHeight;
    int native_width = 0, native_height = 0;

#if TARGET_WEBOS
    if (parent->panel_width > 0 && parent->panel_height > 0 &&
        (max_width == 0 || max_height == 0 || parent->panel_width < max_width || parent->panel_height < max_height)) {
        native_width = max_width = parent->panel_width;
        native_height = max_height = parent->panel_height;
    }
#endif

    lv_obj_t *res_dropdown = pref_dropdown_res(view, max_width, max_height, native_width, native_height,
                                               &app_configuration->stream.width, &app_configuration->stream.height);
    lv_obj_set_width(res_dropdown, LV_PCT(60));
    lv_obj_add_event_cb(res_dropdown, on_res_fps_updated, LV_EVENT_VALUE_CHANGED, self);

    unsigned int max_fps = app->ss4s.video_cap.maxFps;
#if TARGET_WEBOS
    if (parent->panel_fps > 0 && (max_fps == 0 || parent->panel_fps < max_fps)) {
        max_fps = parent->panel_fps;
    }
#endif
    const static int fps_options[] = {30, 60, 90, 120, 144, 240, 0};
    lv_obj_t *fps_dropdown = pref_dropdown_fps(view, fps_options, (int) max_fps, &app_configuration->stream.fps);
    lv_obj_set_flex_grow(fps_dropdown, 1);
    lv_obj_add_event_cb(fps_dropdown, on_res_fps_updated, LV_EVENT_VALUE_CHANGED, self);

    pane->res_warning = lv_label_create(view);
    lv_obj_add_flag(pane->res_warning, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_width(pane->res_warning, LV_PCT(100));
    lv_obj_set_style_text_font(pane->res_warning, lv_theme_get_font_small(view), 0);
    lv_obj_set_style_text_color(pane->res_warning, lv_palette_main(LV_PALETTE_AMBER), 0);
    lv_label_set_long_mode(pane->res_warning, LV_LABEL_LONG_WRAP);

    pane->bitrate_label = pref_title_label(view, locstr("Video bitrate"));

    unsigned int max = app->ss4s.video_cap.maxBitrate ? app->ss4s.video_cap.maxBitrate : 100000;
    lv_obj_t *bitrate_slider = pref_slider(view, &app_configuration->stream.bitrate, 5000, (int) max, BITRATE_STEP);
    lv_obj_set_width(bitrate_slider, LV_PCT(100));
    lv_obj_add_event_cb(bitrate_slider, on_bitrate_changed, LV_EVENT_VALUE_CHANGED, self);
    pane->bitrate_slider = bitrate_slider;

    pane->bitrate_warning = lv_label_create(view);
    lv_obj_add_flag(pane->bitrate_warning, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_width(pane->bitrate_warning, LV_PCT(100));
    lv_obj_set_style_text_font(pane->bitrate_warning, lv_theme_get_font_small(view), 0);
    lv_obj_set_style_text_color(pane->bitrate_warning, lv_palette_main(LV_PALETTE_AMBER), 0);
    lv_label_set_long_mode(pane->bitrate_warning, LV_LABEL_LONG_WRAP);

#if !FEATURE_FORCE_FULLSCREEN
    lv_obj_t *checkbox = pref_checkbox(view, locstr("Fullscreen UI"), &app_configuration->fullscreen, false);
    if (app->ss4s.video_cap.transform & SS4S_VIDEO_CAP_TRANSFORM_AREA_DEST) {
        lv_obj_add_event_cb(checkbox, on_fullscreen_updated, LV_EVENT_VALUE_CHANGED, pane);
    } else {
        lv_obj_add_state(checkbox, LV_STATE_DISABLED);
        pref_desc_label(view, locstr("Can't use windowed UI for this decoder"), false);
    }
#endif

#ifdef FEATURE_I18N_LANGUAGE_SETTINGS
    lv_obj_t *lang_label = pref_title_label(view, "Language");
    if (strcmp(locstr("Language"), "Language") != 0) {
        lv_label_set_text_fmt(lang_label, "%s (Language)", locstr("Language"));
    }

    lv_obj_t *language_dropdown = pref_dropdown_string(view, pane->lang_entries, pane->lang_entries_len,
                                                       &app_configuration->language);
    lv_obj_add_event_cb(language_dropdown, pref_mark_restart_cb, LV_EVENT_VALUE_CHANGED, pane);
    lv_obj_set_width(language_dropdown, LV_PCT(100));
#endif

    update_bitrate_label(pane);
    update_bitrate_hint(pane);

    return view;
}

static void on_bitrate_changed(lv_event_t *e) {
    basic_pane_t *pane = lv_event_get_user_data(e);
    update_bitrate_label(pane);
    update_bitrate_hint(pane);
}

static void on_res_fps_updated(lv_event_t *e) {
    basic_pane_t *pane = lv_event_get_user_data(e);
    int bitrate = settings_optimal_bitrate(&pane->parent->app->ss4s.video_cap, app_configuration->stream.width,
                                           app_configuration->stream.height, app_configuration->stream.fps);
    if (bitrate > app_configuration->stream.bitrate) {
        lv_slider_set_value(pane->bitrate_slider, bitrate / BITRATE_STEP, LV_ANIM_OFF);
        app_configuration->stream.bitrate = lv_slider_get_value(pane->bitrate_slider) * BITRATE_STEP;
    }
    if (app_configuration->stream.width > 1920 && app_configuration->stream.height > 1080 &&
        app_configuration->stream.fps > 60) {
        lv_obj_clear_flag(pane->res_warning, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text_static(pane->res_warning, locstr("Your computer may not perform well when using this "
                                                           "resolution and framerate."));
    } else {
        lv_obj_add_flag(pane->res_warning, LV_OBJ_FLAG_HIDDEN);
    }
    update_bitrate_label(pane);
    update_bitrate_hint(pane);
}

static void on_fullscreen_updated(lv_event_t *e) {
    basic_pane_t *pane = lv_event_get_user_data(e);
    app_set_fullscreen(pane->parent->app, app_configuration->fullscreen);
}

static void update_bitrate_label(basic_pane_t *pane) {
    lv_label_set_text_fmt(pane->bitrate_label, locstr("Video bitrate - %d kbps"), app_configuration->stream.bitrate);
}

static void update_bitrate_hint(basic_pane_t *pane) {
    app_t *app = pane->parent->app;
    if (app->ss4s.video_cap.suggestedBitrate > 0 &&
        app_configuration->stream.bitrate > app->ss4s.video_cap.suggestedBitrate) {
        lv_obj_clear_flag(pane->bitrate_warning, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text_static(pane->bitrate_warning, locstr("Higher bitrate may cause performance issue, "
                                                               "try with caution."));
    } else {
        lv_obj_add_flag(pane->bitrate_warning, LV_OBJ_FLAG_HIDDEN);
    }
}

static void init_locale_entries(basic_pane_t *pane) {
    pane->lang_entries = lv_mem_alloc(sizeof(pref_dropdown_string_entry_t) * (I18N_LOCALES_LEN + 2));
    lv_memset_00(pane->lang_entries, sizeof(pref_dropdown_string_entry_t) * (I18N_LOCALES_LEN + 2));
    for (int i = 0; i < 2; i++) {
        pref_dropdown_string_entry_t *def_entry = &pane->lang_entries[i];
        const i18n_entry_t *entry = i18n_entry_at(i);
        def_entry->value = entry->locale;
        def_entry->name = locstr(entry->name);
        def_entry->fallback = i == 0;
        pane->lang_entries_len++;
    }
    char *input = strdup(I18N_LOCALES), *tok = NULL, *saveptr = input;
    while ((tok = strtok_r(saveptr, ";", &saveptr)) != NULL) {
        const i18n_entry_t *entry = i18n_entry(tok);
        if (entry) {
            pref_dropdown_string_entry_t *pref_entry = &pane->lang_entries[pane->lang_entries_len];
            pref_entry->value = entry->locale;
            pref_entry->name = entry->name;
            pane->lang_entries_len++;
        }
    }
    free(input);
}

static void pref_mark_restart_cb(lv_event_t *e) {
    basic_pane_t *pane = (basic_pane_t *) lv_event_get_user_data(e);
    settings_controller_t *parent = pane->parent;
    parent->needs_restart |= strcasecmp(i18n_locale(), app_configuration->language) != 0;
}
