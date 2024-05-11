#include "app.h"

#include "pref_obj.h"
#include "av_pane.h"

#include "ui/settings/settings.controller.h"

#include "util/i18n.h"
#include "ss4s.h"
#include "ss4s_modules.h"

typedef struct video_pane_t {
    lv_fragment_t base;
    settings_controller_t *parent;

    lv_obj_t *conflict_hint;
    lv_obj_t *hdr_checkbox;
    lv_obj_t *hdr_hint;

    pref_dropdown_string_entry_t *vdec_entries;
    int vdec_entries_len;
} video_pane_t;

static lv_obj_t *create_obj(lv_fragment_t *self, lv_obj_t *container);

static void obj_created(lv_fragment_t *self, lv_obj_t *obj);

static void pane_ctor(lv_fragment_t *self, void *args);

static void pane_dtor(lv_fragment_t *self);

static void module_changed_cb(lv_event_t *e);

static void hdr_state_update_cb(lv_event_t *e);

static void hdr_more_click_cb(lv_event_t *e);

static void hdr_state_update(video_pane_t *controller);

const lv_fragment_class_t settings_pane_video_cls = {
        .constructor_cb = pane_ctor,
        .destructor_cb = pane_dtor,
        .create_obj_cb = create_obj,
        .obj_created_cb = obj_created,
        .instance_size = sizeof(video_pane_t),
};

static void pane_ctor(lv_fragment_t *self, void *args) {
    video_pane_t *pane = (video_pane_t *) self;
    pane->parent = args;
    array_list_t modules = pane->parent->app->ss4s.modules;
    pane->vdec_entries = calloc(modules.size + 1, sizeof(pref_dropdown_string_entry_t));

    set_decoder_entry(&pane->vdec_entries[pane->vdec_entries_len++], locstr("Auto"), "auto", true);
    for (int module_idx = 0; module_idx < modules.size; module_idx++) {
        const SS4S_ModuleInfo *info = array_list_get(&modules, module_idx);
        const char *group = SS4S_ModuleInfoGetGroup(info);
        if (info->has_video && !contains_decoder_group(pane->vdec_entries, pane->vdec_entries_len, group)) {
            set_decoder_entry(&pane->vdec_entries[pane->vdec_entries_len++], info->name, group, false);
        }
    }
}

static void pane_dtor(lv_fragment_t *self) {
    video_pane_t *pane = (video_pane_t *) self;
    free(pane->vdec_entries);
}

static lv_obj_t *create_obj(lv_fragment_t *self, lv_obj_t *container) {
    video_pane_t *controller = (video_pane_t *) self;
    app_t *app = controller->parent->app;
    lv_obj_t *view = pref_pane_container(container);
    lv_obj_set_layout(view, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(view, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(view, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *decoder_label = pref_title_label(view, locstr("Video decoder"));
    lv_label_set_text_fmt(decoder_label, locstr("Video decoder - using %s"),
                          SS4S_ModuleInfoGetName(app->ss4s.selection.video_module));
    lv_obj_t *vdec_dropdown = pref_dropdown_string(view, controller->vdec_entries, controller->vdec_entries_len,
                                                   &app_configuration->decoder);
    lv_obj_set_width(vdec_dropdown, LV_PCT(100));
#if FEATURE_EMBEDDED_SHELL
    if (!app_is_decoder_valid(app) && app_has_embedded(app)) {
        lv_obj_add_flag(decoder_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(vdec_dropdown, LV_OBJ_FLAG_HIDDEN);
    }
#endif

    lv_obj_t *conflict_hint = pref_desc_label(view, NULL, false);
    controller->conflict_hint = conflict_hint;

    pref_header(view, locstr("Video Settings"));

    lv_obj_t *hevc_checkbox = pref_checkbox(view, locstr("Use H265 when possible"), &app_configuration->hevc, false);
    lv_obj_t *hevc_hint = pref_desc_label(view, NULL, false);
    if (app->ss4s.video_cap.codecs & SS4S_VIDEO_H265) {
        lv_obj_clear_state(hevc_checkbox, LV_STATE_DISABLED);
        lv_label_set_text(hevc_hint, locstr("H265 usually has better graphics, and supports HDR."));
    } else {
        lv_obj_add_state(hevc_checkbox, LV_STATE_DISABLED);
        lv_label_set_text_fmt(hevc_hint, locstr("%s decoder doesn't support H265 codec."),
                              SS4S_ModuleInfoGetName(app->ss4s.selection.video_module));
    }

    lv_obj_t *av1_checkbox = pref_checkbox(view, locstr("Use AV1 when possible (Experimental)"),
                                           &app_configuration->av1, false);
    lv_obj_t *av1_hint = pref_desc_label(view, NULL, false);
    if (app->ss4s.video_cap.codecs & SS4S_VIDEO_AV1) {
        lv_obj_clear_state(av1_checkbox, LV_STATE_DISABLED);
        lv_label_set_text(av1_hint, locstr("AV1 usually has better graphics, and supports HDR. "
                                           "However, it might be much slower than H265 or H264."));
    } else {
        lv_obj_add_state(av1_checkbox, LV_STATE_DISABLED);
        lv_label_set_text_fmt(av1_hint, locstr("%s decoder doesn't support AV1 codec."),
                              SS4S_ModuleInfoGetName(app->ss4s.selection.video_module));
    }

    lv_obj_t *hdr_checkbox = pref_checkbox(view, locstr("HDR"), &app_configuration->hdr, false);
    lv_obj_t *hdr_hint = pref_desc_label(view, NULL, false);
    controller->hdr_checkbox = hdr_checkbox;
    controller->hdr_hint = hdr_hint;

    hdr_state_update(controller);

    lv_obj_t *hdr_more = pref_desc_label(view, locstr("Learn more about HDR feature."), true);
    lv_obj_set_style_text_color(hdr_more, lv_theme_get_color_primary(hdr_more), 0);
    lv_obj_add_flag(hdr_more, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_add_event_cb(vdec_dropdown, module_changed_cb, LV_EVENT_VALUE_CHANGED, controller);
    lv_obj_add_event_cb(hevc_checkbox, hdr_state_update_cb, LV_EVENT_VALUE_CHANGED, controller);
    lv_obj_add_event_cb(av1_checkbox, hdr_state_update_cb, LV_EVENT_VALUE_CHANGED, controller);
    lv_obj_add_event_cb(hdr_more, hdr_more_click_cb, LV_EVENT_CLICKED, NULL);

    return view;
}

static void obj_created(lv_fragment_t *self, lv_obj_t *obj) {
    video_pane_t *fragment = (video_pane_t *) self;
    update_conflict_hint(fragment->parent->app, fragment->conflict_hint);
}

static void module_changed_cb(lv_event_t *e) {
    video_pane_t *fragment = (video_pane_t *) lv_event_get_user_data(e);
    settings_controller_t *parent = fragment->parent;
    parent->needs_restart = true;
    update_conflict_hint(parent->app, fragment->conflict_hint);
}

static void hdr_state_update_cb(lv_event_t *e) {
    video_pane_t *controller = (video_pane_t *) lv_event_get_user_data(e);
    hdr_state_update(controller);
}

static void hdr_state_update(video_pane_t *controller) {
    app_t *app = controller->parent->app;
    if (app->ss4s.video_cap.hdr == 0) {
        lv_obj_add_state(controller->hdr_checkbox, LV_STATE_DISABLED);
        lv_label_set_text_fmt(controller->hdr_hint, locstr("%s decoder doesn't support HDR."),
                              SS4S_ModuleInfoGetName(app->ss4s.selection.video_module));
    } else if (!app_configuration->hevc && !app_configuration->av1) {
        lv_obj_add_state(controller->hdr_checkbox, LV_STATE_DISABLED);
        lv_label_set_text(controller->hdr_hint, locstr("H265 or AV1 is required to use HDR."));
    } else {
        lv_obj_clear_state(controller->hdr_checkbox, LV_STATE_DISABLED);
        lv_label_set_text(controller->hdr_hint, locstr("HDR is only supported on certain games and "
                                                       "when connecting to supported monitor."));
    }
}

static void hdr_more_click_cb(lv_event_t *e) {
    (void) e;
    app_open_url("https://github.com/mariotaku/moonlight-tv/wiki/HDR-Support");
}