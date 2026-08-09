#include "app.h"
#include "config.h"

#include "pref_obj.h"
#include "pref_fps.h"
#include "pref_res.h"
#include "ui/settings/settings.controller.h"

#include "util/i18n.h"
#include "logging.h"

typedef enum {
    BITRATE_WARN_NONE,
    BITRATE_WARN_SUGGESTED,
    BITRATE_WARN_MAX,
} bitrate_warn_t;

typedef struct {
    lv_fragment_t base;
    settings_controller_t *parent;

    lv_obj_t *res_warning;
    lv_obj_t *bitrate_label;
    lv_obj_t *bitrate_slider;
    lv_obj_t *bitrate_warning;
    // The slider spans one step past BITRATE_MAX, which selects "unlimited". It writes here rather
    // than straight into the config so that stream.bitrate never holds that extra step.
    int bitrate_slider_value;
    bitrate_warn_t bitrate_warn;

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

// One step past the last real value; selecting it means "unlimited".
#define BITRATE_SLIDER_MAX (BITRATE_MAX + BITRATE_STEP)

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
    int native_width = app->ui.width, native_height = app->ui.height;

#if TARGET_WEBOS
    if (parent->panel_width > 0 && parent->panel_height > 0) {
        native_width = parent->panel_width;
        native_height = parent->panel_height;
    }

    commons_log_info("Settings", "Panel native resolution: %d x %d, maximum video resolution: %d x %d",
                     native_width, native_height, max_width, max_height);
#endif
    if (max_width == 0 || max_height == 0) {
        max_width = native_width;
        max_height = native_height;
    }

    lv_obj_t *res_dropdown = pref_dropdown_res(view, native_width, native_height, max_width, max_height,
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

    // Every model gets the same slider; the per-decoder maxBitrate feeds update_bitrate_hint
    // rather than shortening the range.
    pane->bitrate_slider_value = app_configuration->bitrate_unlimited ? BITRATE_SLIDER_MAX
                                                                     : app_configuration->stream.bitrate;
    lv_obj_t *bitrate_slider = pref_slider(view, &pane->bitrate_slider_value, BITRATE_MIN, BITRATE_SLIDER_MAX,
                                           BITRATE_STEP);
    lv_obj_set_width(bitrate_slider, LV_PCT(100));
    lv_obj_add_event_cb(bitrate_slider, on_bitrate_changed, LV_EVENT_VALUE_CHANGED, self);
    pane->bitrate_slider = bitrate_slider;

    pane->bitrate_warning = lv_label_create(view);
    lv_obj_add_flag(pane->bitrate_warning, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_width(pane->bitrate_warning, LV_PCT(100));
    lv_obj_set_style_text_font(pane->bitrate_warning, lv_theme_get_font_small(view), 0);
    lv_label_set_long_mode(pane->bitrate_warning, LV_LABEL_LONG_WRAP);
    pane->bitrate_warn = BITRATE_WARN_NONE;

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
    app_configuration->bitrate_unlimited = pane->bitrate_slider_value > BITRATE_MAX;
    if (!app_configuration->bitrate_unlimited) {
        app_configuration->stream.bitrate = pane->bitrate_slider_value;
    }
    update_bitrate_label(pane);
    update_bitrate_hint(pane);
}

static void on_res_fps_updated(lv_event_t *e) {
    basic_pane_t *pane = lv_event_get_user_data(e);
    int bitrate = settings_optimal_bitrate(&pane->parent->app->ss4s.video_cap, app_configuration->stream.width,
                                           app_configuration->stream.height, app_configuration->stream.fps);
    // Without capabilities to cap it this can exceed the slider, and letting it reach the last step
    // would select "unlimited" for a user who only changed the resolution.
    if (bitrate > BITRATE_MAX) {
        bitrate = BITRATE_MAX;
    }
    if (!app_configuration->bitrate_unlimited && bitrate > app_configuration->stream.bitrate) {
        app_configuration->stream.bitrate = bitrate;
        pane->bitrate_slider_value = bitrate;
        lv_slider_set_value(pane->bitrate_slider, bitrate / BITRATE_STEP, LV_ANIM_OFF);
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
    if (app_configuration->bitrate_unlimited) {
        lv_label_set_text(pane->bitrate_label, locstr("Video bitrate - Unlimited"));
    } else {
        lv_label_set_text_fmt(pane->bitrate_label, locstr("Video bitrate - %d kbps"),
                              app_configuration->stream.bitrate);
    }
}

static void update_bitrate_hint(basic_pane_t *pane) {
    const SS4S_VideoCapabilities *cap = &pane->parent->app->ss4s.video_cap;
    int max = (int) cap->maxBitrate, suggested = (int) cap->suggestedBitrate;
    int bitrate = settings_effective_bitrate(app_configuration, cap);
    // A decoder that declares no ceiling gives us no figure to compare against, but "unlimited"
    // still deserves the strong warning -- it is the state the old slider range made unreachable.
    bitrate_warn_t warn = BITRATE_WARN_NONE;
    if (max > 0 ? bitrate > max : app_configuration->bitrate_unlimited) {
        warn = BITRATE_WARN_MAX;
    } else if (suggested > 0 && bitrate > suggested) {
        warn = BITRATE_WARN_SUGGESTED;
    }
    // The branch changes at most twice across a slider sweep, so don't re-style and re-wrap the
    // label on every step of the D-pad.
    if (warn == pane->bitrate_warn) {
        return;
    }
    pane->bitrate_warn = warn;
    switch (warn) {
        case BITRATE_WARN_MAX: {
            const char *text = locstr("May exceed what this decoder can handle. The stream may stutter or fail "
                                      "to start.");
            lv_obj_set_style_text_color(pane->bitrate_warning, lv_palette_main(LV_PALETTE_RED), 0);
            // Keep the format string a literal so a mistranslated conversion can't reach vsnprintf.
            if (max > 0) {
                lv_label_set_text_fmt(pane->bitrate_warning, "%s (%d kbps)", text, max);
            } else {
                lv_label_set_text(pane->bitrate_warning, text);
            }
            lv_obj_clear_flag(pane->bitrate_warning, LV_OBJ_FLAG_HIDDEN);
            break;
        }
        case BITRATE_WARN_SUGGESTED:
            lv_obj_set_style_text_color(pane->bitrate_warning, lv_palette_main(LV_PALETTE_AMBER), 0);
            lv_label_set_text(pane->bitrate_warning, locstr("Higher bitrate may cause performance issue, "
                                                            "try with caution."));
            lv_obj_clear_flag(pane->bitrate_warning, LV_OBJ_FLAG_HIDDEN);
            break;
        default:
            lv_obj_add_flag(pane->bitrate_warning, LV_OBJ_FLAG_HIDDEN);
            break;
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
