#include "app.h"
#include "pref_obj.h"

#include <stream/platform.h>

#include "util/i18n.h"

typedef struct {
    lv_obj_controller_t base;
    lv_obj_t *res_warning;
    lv_obj_t *bitrate_label;
    lv_obj_t *bitrate_slider;
} basic_pane_t;

static void pane_ctor(lv_obj_controller_t *self, void *args);

static lv_obj_t *create_obj(lv_obj_controller_t *self, lv_obj_t *parent);

static void on_bitrate_changed(lv_event_t *e);

static void on_res_fps_updated(lv_event_t *e);

static void on_fullscreen_updated(lv_event_t *e);

static void update_bitrate_label(basic_pane_t *pane);

const lv_obj_controller_class_t settings_pane_basic_cls = {
        .constructor_cb = pane_ctor,
        .create_obj_cb = create_obj,
        .instance_size = sizeof(basic_pane_t),
};
static const pref_dropdown_int_pair_entry_t supported_resolutions[] = {
        {"720P",  1280, 720, true},
        {"1080P", 1920, 1080},
        {"1440P", 2560, 1440},
        {"4K",    3840, 2160},
//        {"Automatic", 0,    0, true},
};

static const int supported_resolutions_len = sizeof(supported_resolutions) / sizeof(pref_dropdown_int_pair_entry_t);

static const pref_dropdown_int_entry_t supported_fps[] = {
        {"30 FPS",  30},
        {"60 FPS",  60, true},
        {"90 FPS",  90},
        {"120 FPS", 120},
};
static const int supported_fps_len = sizeof(supported_fps) / sizeof(pref_dropdown_int_entry_t);
#define BITRATE_STEP 1000

static void pane_ctor(lv_obj_controller_t *self, void *args) {

}

static lv_obj_t *create_obj(lv_obj_controller_t *self, lv_obj_t *parent) {
    lv_obj_set_layout(parent, LV_LAYOUT_GRID);
    basic_pane_t *pane = (basic_pane_t *) self;
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_ROW_WRAP);
    pref_title_label(parent, locstr("Resolution and FPS"));

    lv_obj_t *resolution_dropdown = pref_dropdown_int_pair(parent, supported_resolutions, supported_resolutions_len,
                                                           &app_configuration->stream.width,
                                                           &app_configuration->stream.height);
    lv_obj_set_width(resolution_dropdown, LV_PCT(60));
    lv_obj_add_event_cb(resolution_dropdown, on_res_fps_updated, LV_EVENT_VALUE_CHANGED, self);

    int max_fps = decoder_max_framerate();
    int fps_len;
    for (fps_len = supported_fps_len; fps_len > 0; fps_len--) {
        if (max_fps == 0 || supported_fps[fps_len - 1].value <= max_fps) break;
    }
    lv_obj_t *fps_dropdown = pref_dropdown_int(parent, supported_fps, fps_len, &app_configuration->stream.fps);
    lv_obj_set_flex_grow(fps_dropdown, 1);
    lv_obj_add_event_cb(fps_dropdown, on_res_fps_updated, LV_EVENT_VALUE_CHANGED, self);

    pane->res_warning = lv_label_create(parent);
    lv_obj_add_flag(pane->res_warning, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_width(pane->res_warning, LV_PCT(100));
    lv_obj_set_style_text_font(pane->res_warning, lv_theme_get_font_small(parent), 0);
    lv_obj_set_style_text_color(pane->res_warning, lv_palette_main(LV_PALETTE_AMBER), 0);
    lv_label_set_long_mode(pane->res_warning, LV_LABEL_LONG_WRAP);

    pane->bitrate_label = pref_title_label(parent, locstr("Video bitrate"));
    update_bitrate_label(pane);

    int max = decoder_info.maxBitrate ? decoder_info.maxBitrate : 50000;
    lv_obj_t *bitrate_slider = pref_slider(parent, &app_configuration->stream.bitrate, 5000, max, BITRATE_STEP);
    lv_obj_set_width(bitrate_slider, LV_PCT(100));
    lv_obj_add_event_cb(bitrate_slider, on_bitrate_changed, LV_EVENT_VALUE_CHANGED, self);
    pane->bitrate_slider = bitrate_slider;

#if !FEATURE_FORCE_FULLSCREEN
    lv_obj_t *checkbox = pref_checkbox(parent, locstr("Fullscreen UI"), &app_configuration->fullscreen, false);
    lv_obj_add_event_cb(checkbox, on_fullscreen_updated, LV_EVENT_VALUE_CHANGED, NULL);
#endif

    return NULL;
}

static void on_bitrate_changed(lv_event_t *e) {
    basic_pane_t *pane = lv_event_get_user_data(e);
    update_bitrate_label(pane);
}

static void on_res_fps_updated(lv_event_t *e) {
    basic_pane_t *pane = lv_event_get_user_data(e);
    int bitrate = settings_optimal_bitrate(app_configuration->stream.width, app_configuration->stream.height,
                                           app_configuration->stream.fps);
    lv_slider_set_value(pane->bitrate_slider, bitrate / BITRATE_STEP, LV_ANIM_OFF);
    app_configuration->stream.bitrate = lv_slider_get_value(pane->bitrate_slider) * BITRATE_STEP;
    update_bitrate_label(pane);
    if (app_configuration->stream.width > 1920 && app_configuration->stream.height > 1080 &&
        app_configuration->stream.fps > 60) {
        lv_obj_clear_flag(pane->res_warning, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text_static(pane->res_warning, locstr("Your computer may not perform well when using this "
                                                           "resolution and framerate."));
    } else {
        lv_obj_add_flag(pane->res_warning, LV_OBJ_FLAG_HIDDEN);
    }
}

static void on_fullscreen_updated(lv_event_t *e) {
    app_set_fullscreen(app_configuration->fullscreen);
}

static void update_bitrate_label(basic_pane_t *pane) {
    lv_label_set_text_fmt(pane->bitrate_label, locstr("Video bitrate - %d kbps"), app_configuration->stream.bitrate);
}
