#include "streaming.controller.h"

#include "lvgl/ext/lv_child_group.h"

#include "util/i18n.h"

static lv_obj_t *stat_label(lv_obj_t *parent, const char *title);

lv_obj_t *streaming_scene_create(lv_fragment_t *self, lv_obj_t *parent) {
    streaming_controller_t *controller = (streaming_controller_t *) self;
    lv_obj_t *scene = lv_obj_create(parent);
    controller->group = lv_group_create();
    controller->scene = scene;
    lv_obj_add_event_cb(scene, cb_child_group_add, LV_EVENT_CHILD_CREATED, controller->group);

    lv_obj_remove_style_all(scene);
    lv_obj_clear_flag(scene, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(scene, LV_PCT(100), LV_PCT(100));

    lv_obj_t *video = lv_obj_create(scene);
    lv_obj_remove_style_all(video);
    lv_obj_set_size(video, LV_PCT(50), LV_PCT(50));
    lv_obj_align(video, LV_ALIGN_TOP_LEFT, LV_DPX(20), LV_DPX(20));
    lv_obj_clear_flag(video, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *kbd_btn = lv_btn_create(scene);
    lv_obj_add_flag(kbd_btn, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_style(kbd_btn, &controller->overlay_button_style, 0);
    lv_obj_set_style_bg_color(kbd_btn, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_t *kbd_label = lv_label_create(kbd_btn);
    lv_obj_add_style(kbd_label, &controller->overlay_button_label_style, 0);
    lv_label_set_text(kbd_label, locstr("Soft keyboard"));

    lv_obj_t *vmouse_btn = lv_btn_create(scene);
    lv_obj_add_flag(vmouse_btn, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_style(vmouse_btn, &controller->overlay_button_style, 0);
    lv_obj_set_style_bg_color(vmouse_btn, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_t *vmouse_label = lv_label_create(vmouse_btn);
    lv_obj_add_style(vmouse_label, &controller->overlay_button_label_style, 0);
    lv_label_set_text(vmouse_label, locstr("Virtual Mouse"));

    lv_obj_t *suspend_btn = lv_btn_create(scene);
    lv_obj_add_flag(suspend_btn, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_style(suspend_btn, &controller->overlay_button_style, 0);
    lv_obj_set_style_bg_color(suspend_btn, lv_palette_main(LV_PALETTE_AMBER), 0);
    lv_obj_t *suspend_lbl = lv_label_create(suspend_btn);
    lv_obj_add_style(suspend_lbl, &controller->overlay_button_label_style, 0);
    lv_label_set_text(suspend_lbl, locstr("Disconnect"));

    lv_obj_t *exit_btn = lv_btn_create(scene);
    lv_obj_add_flag(exit_btn, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_style(exit_btn, &controller->overlay_button_style, 0);
    lv_obj_set_style_bg_color(exit_btn, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_t *exit_lbl = lv_label_create(exit_btn);
    lv_obj_add_style(exit_lbl, &controller->overlay_button_label_style, 0);
    lv_label_set_text(exit_lbl, locstr("Quit game"));

    lv_obj_t *stats = lv_obj_create(scene);
    lv_obj_remove_style_all(stats);
    lv_obj_set_style_pad_all(stats, LV_DPX(15), 0);
    lv_obj_set_style_pad_gap(stats, LV_DPX(5), 0);
    lv_obj_set_style_bg_opa(stats, LV_OPA_40, 0);
    lv_obj_set_style_bg_color(stats, lv_color_black(), 0);
    lv_obj_set_size(stats, LV_PCT(40), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(stats, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_align(stats, LV_ALIGN_TOP_RIGHT, -LV_DPX(20), LV_DPX(20));

    controller->stats_items.resolution = stat_label(stats, "Resolution");
    controller->stats_items.decoder = stat_label(stats, "Decoder");
    controller->stats_items.audio = stat_label(stats, "Audio backend");

    controller->stats_items.rtt = stat_label(stats, "Network RTT");
    controller->stats_items.net_fps = stat_label(stats, "Network framerate");
    controller->stats_items.drop_rate = stat_label(stats, "Network frame drop");
    controller->stats_items.decode_time = stat_label(stats, "Decode time");


    lv_obj_add_flag(scene, LV_OBJ_FLAG_HIDDEN);

    controller->video = video;
    controller->kbd_btn = kbd_btn;
    controller->vmouse_btn = vmouse_btn;
    controller->quit_btn = exit_btn;
    controller->suspend_btn = suspend_btn;
    controller->stats = stats;

    streaming_overlay_resized(controller);

    return scene;
}

void streaming_styles_init(streaming_controller_t *controller) {
    lv_theme_t *theme = lv_disp_get_default()->theme;

    lv_style_init(&controller->overlay_button_style);
    lv_style_set_shadow_ofs_y(&controller->overlay_button_style, LV_DPX(3));
    lv_style_set_shadow_width(&controller->overlay_button_style, LV_DPX(4));
    lv_style_set_shadow_color(&controller->overlay_button_style, lv_color_black());
    lv_style_set_shadow_opa(&controller->overlay_button_style, LV_OPA_30);
    lv_style_set_radius(&controller->overlay_button_style, LV_DPX(8));
    lv_style_set_pad_hor(&controller->overlay_button_style, LV_DPX(15));
    lv_style_set_pad_ver(&controller->overlay_button_style, LV_DPX(10));

    lv_style_init(&controller->overlay_button_label_style);
    lv_style_set_text_font(&controller->overlay_button_label_style, theme->font_small);
}

void streaming_overlay_resized(streaming_controller_t *controller) {
    lv_obj_align(controller->kbd_btn, LV_ALIGN_BOTTOM_LEFT, LV_DPX(20), -LV_DPX(20));
    lv_obj_align(controller->quit_btn, LV_ALIGN_BOTTOM_RIGHT, -LV_DPX(20), -LV_DPX(20));

    lv_obj_align_to(controller->vmouse_btn, controller->kbd_btn, LV_ALIGN_OUT_RIGHT_MID, LV_DPX(10), 0);
    lv_obj_align_to(controller->suspend_btn, controller->quit_btn, LV_ALIGN_OUT_LEFT_MID, -LV_DPX(10), 0);

    lv_obj_update_layout(controller->scene);
}

lv_obj_t *progress_dialog_create(const char *message) {
    lv_obj_t *dialog = lv_msgbox_create(NULL, NULL, NULL, NULL, false);
    lv_obj_t *content = lv_msgbox_get_content(dialog);
    lv_obj_set_layout(content, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t *spinner = lv_spinner_create(content, 1000, 60);
    lv_obj_set_style_arc_width(spinner, lv_dpx(10), 0);
    lv_obj_set_style_arc_width(spinner, lv_dpx(10), LV_PART_INDICATOR);
    lv_obj_set_size(spinner, lv_dpx(50), lv_dpx(50));
    lv_obj_set_flex_grow(spinner, 0);
    lv_obj_t *label = lv_label_create(content);
    lv_obj_set_style_pad_hor(label, LV_DPX(20), 0);
    lv_label_set_text(label, message);
    lv_obj_set_flex_grow(label, 1);

    lv_obj_center(dialog);
    return dialog;
}

static lv_obj_t *stat_label(lv_obj_t *parent, const char *title) {
    lv_obj_t *container = lv_obj_create(parent);
    lv_obj_remove_style_all(container);
    lv_obj_set_size(container, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_flex_main_place(container, LV_FLEX_ALIGN_SPACE_BETWEEN, 0);
    lv_obj_t *label = lv_label_create(container);
    lv_label_set_text(label, title);
    lv_obj_set_style_text_font(label, lv_theme_get_font_small(container), 0);
    lv_obj_t *value = lv_label_create(container);
    lv_obj_set_style_text_font(value, lv_theme_get_font_small(container), 0);
    return value;
}
