#include <ui/manager.h>
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

#if TARGET_RASPI
#define OVERLAY_WINDOW_FLAGS NK_WINDOW_TITLE | NK_WINDOW_NO_SCROLLBAR
#else
#define OVERLAY_WINDOW_FLAGS NK_WINDOW_TITLE | NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_MINIMIZABLE | NK_WINDOW_MOVABLE
#endif

void streaming_overlay_init() {
    stream_overlay_showing = false;
}

bool streaming_overlay_dispatch_userevent(int which) {
    switch (which) {
        case USER_ST_QUITAPP_CONFIRM:
            streaming_overlay_show();
            return true;
        default:
            break;
    }
    return false;
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
    struct nk_vec2 wndpos = nk_vec2_s(10, 10);

    streaming_enter_overlay(ui_display_width / 2 - wndpos.x, wndpos.y, ui_display_width / 2, ui_display_height / 2);
    return true;
}

lv_obj_t *streaming_scene_create(streaming_controller_t *controller, lv_obj_t *parent) {
    lv_obj_t *scene = lv_obj_create(parent);
    lv_obj_set_style_radius(scene, 0, 0);
    lv_obj_set_style_border_side(scene, LV_BORDER_SIDE_NONE, 0);
    lv_obj_set_style_bg_opa(scene, 0, 0);
    lv_obj_set_size(scene, LV_PCT(100), LV_PCT(100));
    lv_obj_t *progress = lv_spinner_create(scene, 1000, 60);
    lv_obj_set_size(progress, lv_dpx(50), lv_dpx(50));
    lv_obj_center(progress);

    controller->progress = progress;
    return scene;
}