#include <lvgl/lv_obj_controller.h>
#include "overlay.h"
#include "ui/root.h"
#include "ui/messages.h"

#include "stream/platform.h"
#include "stream/session.h"
#include "stream/video/delegate.h"
#include "util/bus.h"
#include "util/user_event.h"
#include "streaming.controller.h"

bool stream_overlay_showing;


void streaming_overlay_init() {
    stream_overlay_showing = false;
}

bool streaming_overlay_should_block_input() {
    return stream_overlay_showing;
}

bool streaming_overlay_hide() {
    if (!stream_overlay_showing)
        return false;
    stream_overlay_showing = false;
    streaming_enter_fullscreen();
    return true;
}

bool streaming_overlay_show() {
    if (stream_overlay_showing)
        return false;
    stream_overlay_showing = true;

    streaming_enter_overlay(0, 0, ui_display_width / 2, ui_display_height / 2);
    return true;
}

lv_obj_t *streaming_scene_create(lv_obj_controller_t *self, lv_obj_t *parent) {
    streaming_controller_t *controller = (streaming_controller_t *) self;
    lv_obj_t *scene = lv_obj_create(parent);
    lv_obj_set_style_radius(scene, 0, 0);
    lv_obj_set_style_border_side(scene, LV_BORDER_SIDE_NONE, 0);
    lv_obj_set_style_bg_opa(scene, 0, 0);
    lv_obj_set_size(scene, LV_PCT(100), LV_PCT(100));
    lv_obj_t *progress = lv_spinner_create(scene, 1000, 60);
    lv_obj_set_size(progress, lv_dpx(50), lv_dpx(50));
    lv_obj_center(progress);

    lv_obj_t *exit_btn = lv_btn_create(scene);
    lv_obj_t *exit_lbl = lv_label_create(exit_btn);
    lv_label_set_text(exit_lbl, "Exit");
    lv_obj_align(exit_btn, LV_ALIGN_BOTTOM_RIGHT, 0, 0);

    lv_obj_t *suspend_btn = lv_btn_create(scene);
    lv_obj_t *suspend_lbl = lv_label_create(suspend_btn);
    lv_label_set_text(suspend_lbl, "Games List");
    lv_obj_align_to(suspend_btn, exit_btn, LV_ALIGN_OUT_LEFT_MID, 0, 0);


    controller->quit_btn = exit_btn;
    controller->suspend_btn = suspend_btn;

    controller->progress = progress;
    return scene;
}