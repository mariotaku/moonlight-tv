#include "app.h"
#include "streaming.controller.h"
#include "stream/video/delegate.h"
#include "stream/platform.h"
#include "ui/root.h"
#include "lvgl/util/lv_app_utils.h"
#include "lvgl/lv_ext_utils.h"
#include "stream/input/absinput.h"

#include "util/user_event.h"
#include "util/i18n.h"
#include "ui/common/progress_dialog.h"

static void exit_streaming(lv_event_t *event);

static void suspend_streaming(lv_event_t *event);

static void open_keyboard(lv_event_t *event);

static void toggle_vmouse(lv_event_t *event);

static void hide_overlay(lv_event_t *event);

static bool show_overlay(streaming_controller_t *controller);

static void on_view_created(lv_fragment_t *self, lv_obj_t *view);

static void on_delete_obj(lv_fragment_t *self, lv_obj_t *view);

static bool on_event(lv_fragment_t *self, int code, void *userdata);

static void constructor(lv_fragment_t *self, void *args);

static void controller_dtor(lv_fragment_t *self);

static void session_error(streaming_controller_t *controller);

static void session_error_dialog_cb(lv_event_t *event);

static void overlay_key_cb(lv_event_t *e);

static void update_buttons_layout(streaming_controller_t *controller);

bool stats_showing(streaming_controller_t *controller);

const lv_fragment_class_t streaming_controller_class = {
        .constructor_cb = constructor,
        .destructor_cb = controller_dtor,
        .create_obj_cb = streaming_scene_create,
        .obj_created_cb = on_view_created,
        .obj_will_delete_cb = on_delete_obj,
        .event_cb = on_event,
        .instance_size = sizeof(streaming_controller_t),
};

static bool overlay_showing;
static streaming_controller_t *current_controller = NULL;

bool streaming_overlay_shown() {
    return overlay_showing;
}

bool stats_showing(streaming_controller_t *controller) {
    return streaming_overlay_shown() || controller->stats->parent != controller->overlay;
}

bool streaming_refresh_stats() {
    streaming_controller_t *controller = current_controller;
    if (!controller) return false;
    if (!stats_showing(controller)) {
        return false;
    }
    app_t *app = controller->global;
    const struct VIDEO_STATS *dst = &vdec_summary_stats;
    const struct VIDEO_INFO *info = &vdec_stream_info;
    if (info->width > 0 && info->height > 0) {
        lv_label_set_text_fmt(controller->stats_items.resolution, "%d * %d", info->width, info->height);
    } else {
        lv_label_set_text(controller->stats_items.resolution, "N/A");
    }
    lv_label_set_text_fmt(controller->stats_items.decoder, "%s (%s)", app->ss4s.selection.video_driver,
                          vdec_stream_info.format);
    lv_label_set_text_static(controller->stats_items.audio, app->ss4s.selection.audio_driver);
    lv_label_set_text_fmt(controller->stats_items.rtt, "%d ms (var. %d ms)", dst->rtt, dst->rttVariance);
    lv_label_set_text_fmt(controller->stats_items.net_fps, "%.2f FPS", dst->receivedFps);

    if (dst->decodedFrames) {
        lv_label_set_text_fmt(controller->stats_items.drop_rate, "%.2f%%",
                              (float) dst->networkDroppedFrames / (float) dst->totalFrames * 100);
        lv_label_set_text_fmt(controller->stats_items.decode_time, "%.2f ms",
                              (float) dst->totalDecodeTime / (float) dst->decodedFrames);
    } else {
        lv_label_set_text(controller->stats_items.drop_rate, "-");
        lv_label_set_text_fmt(controller->stats_items.decode_time, "-");
    }
    return true;
}

void streaming_notice_show(const char *message) {
    streaming_controller_t *controller = current_controller;
    if (!controller) return;
    lv_label_set_text(controller->notice_label, message);
    if (message && message[0]) {
        lv_obj_clear_flag(controller->notice, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(controller->notice, LV_OBJ_FLAG_HIDDEN);
    }
}

static void constructor(lv_fragment_t *self, void *args) {
    streaming_controller_t *controller = (streaming_controller_t *) self;
    current_controller = controller;

    overlay_showing = false;

    streaming_styles_init(controller);

    const streaming_scene_arg_t *arg = (streaming_scene_arg_t *) args;
    controller->global = arg->global;
    streaming_begin(&arg->uuid, &arg->app);
}

static void controller_dtor(lv_fragment_t *self) {
    streaming_controller_t *fragment = (streaming_controller_t *) self;
    lv_style_reset(&fragment->overlay_button_label_style);
    lv_style_reset(&fragment->overlay_button_style);
    if (current_controller == fragment) {
        current_controller = NULL;
    }
}

static bool on_event(lv_fragment_t *self, int code, void *userdata) {
    LV_UNUSED(userdata);
    streaming_controller_t *controller = (streaming_controller_t *) self;
    switch (code) {
        case USER_STREAM_CONNECTING: {
            controller->progress = progress_dialog_create(locstr("Connecting..."));
            if (lv_obj_check_type(controller->progress->parent, &lv_msgbox_backdrop_class)) {
                lv_obj_set_style_bg_opa(controller->progress->parent, LV_OPA_TRANSP, 0);
            }
            lv_obj_set_width(controller->progress, LV_DPX(300));
            lv_obj_set_height(controller->progress, LV_DPX(100));
            lv_obj_add_flag(controller->overlay, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(controller->hint, LV_OBJ_FLAG_HIDDEN);
            return true;
        }
        case USER_STREAM_OPEN: {
            if (controller->progress) {
                lv_msgbox_close(controller->progress);
                controller->progress = NULL;
            }
            lv_obj_add_flag(controller->overlay, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(controller->hint, LV_OBJ_FLAG_HIDDEN);
            break;
        }
        case USER_STREAM_CLOSE: {
            controller->progress = progress_dialog_create(locstr("Disconnecting..."));
            lv_obj_add_flag(controller->overlay, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(controller->stats, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(controller->hint, LV_OBJ_FLAG_HIDDEN);
            break;
        }
        case USER_STREAM_FINISHED: {
            if (controller->progress) {
                lv_msgbox_close(controller->progress);
                controller->progress = NULL;
            }
            lv_obj_add_flag(controller->overlay, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(controller->hint, LV_OBJ_FLAG_HIDDEN);
            if (streaming_errno != 0) {
                session_error(controller);
                break;
            }
            lv_fragment_del((lv_fragment_t *) controller);
            return true;
        }
        case USER_OPEN_OVERLAY: {
            show_overlay(controller);
            return true;
        }
        case USER_SIZE_CHANGED: {
            update_buttons_layout(controller);
            streaming_overlay_resized(controller);
            return false;
        }
        default: {
            break;
        }
    }
    return false;
}

static void on_view_created(lv_fragment_t *self, lv_obj_t *view) {
    streaming_controller_t *controller = (streaming_controller_t *) self;
    app_input_set_group(controller->group);
    lv_obj_add_event_cb(controller->quit_btn, exit_streaming, LV_EVENT_CLICKED, self);
    lv_obj_add_event_cb(controller->suspend_btn, suspend_streaming, LV_EVENT_CLICKED, self);
    lv_obj_add_event_cb(controller->kbd_btn, open_keyboard, LV_EVENT_CLICKED, self);
    lv_obj_add_event_cb(controller->vmouse_btn, toggle_vmouse, LV_EVENT_CLICKED, self);
    lv_obj_add_event_cb(controller->base.obj, hide_overlay, LV_EVENT_CLICKED, self);
    lv_obj_add_event_cb(controller->overlay, overlay_key_cb, LV_EVENT_KEY, controller);
    lv_obj_add_event_cb(controller->base.obj, hide_overlay, LV_EVENT_CANCEL, controller);

    lv_obj_t *notice = lv_obj_create(lv_layer_sys());
    lv_obj_set_size(notice, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(notice, LV_ALIGN_TOP_RIGHT, -LV_DPX(20), LV_DPX(20));
    lv_obj_set_style_radius(notice, LV_DPX(5), 0);
    lv_obj_set_style_pad_hor(notice, LV_DPX(5), 0);
    lv_obj_set_style_pad_ver(notice, LV_DPX(3), 0);
    lv_obj_set_style_border_opa(notice, LV_OPA_TRANSP, 0);
    lv_obj_set_style_bg_opa(notice, LV_OPA_40, 0);
    lv_obj_set_style_bg_color(notice, lv_color_black(), 0);
    lv_obj_t *notice_label = lv_label_create(notice);
    lv_obj_set_size(notice_label, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_text_font(notice_label, lv_theme_get_font_small(view), 0);
    lv_obj_add_flag(notice, LV_OBJ_FLAG_HIDDEN);

    controller->notice = notice;
    controller->notice_label = notice_label;
}

static void on_delete_obj(lv_fragment_t *self, lv_obj_t *view) {
    LV_UNUSED(view);
    streaming_controller_t *controller = (streaming_controller_t *) self;
    if (controller->notice) {
        lv_obj_del(controller->notice);
    }
    if (controller->stats->parent != controller->overlay) {
        lv_obj_del(controller->stats);
    }
    app_input_set_group(NULL);
    lv_group_del(controller->group);
}

static void exit_streaming(lv_event_t *event) {
    LV_UNUSED(event);
    streaming_interrupt(true, STREAMING_INTERRUPT_USER);
}

static void suspend_streaming(lv_event_t *event) {
    LV_UNUSED(event);
    streaming_interrupt(false, STREAMING_INTERRUPT_USER);
}

static void open_keyboard(lv_event_t *event) {
    LV_UNUSED(event);
    app_start_text_input(0, 0, ui_display_width, ui_display_height);
}

static void toggle_vmouse(lv_event_t *event) {
    LV_UNUSED(event);
    absinput_set_virtual_mouse(!absinput_get_virtual_mouse());
}

bool show_overlay(streaming_controller_t *controller) {
    if (overlay_showing)
        return false;
    overlay_showing = true;
    lv_obj_clear_flag(controller->base.obj, LV_OBJ_FLAG_HIDDEN);

    lv_area_t coords = controller->video->coords;
    streaming_enter_overlay(coords.x1, coords.y1, lv_area_get_width(&coords), lv_area_get_height(&coords));
    streaming_refresh_stats();

    update_buttons_layout(controller);
    return true;
}

static void hide_overlay(lv_event_t *event) {
    streaming_controller_t *controller = (streaming_controller_t *) lv_event_get_user_data(event);
    app_input_set_button_points(NULL);
    lv_obj_add_flag(controller->base.obj, LV_OBJ_FLAG_HIDDEN);
    if (!overlay_showing)
        return;
    overlay_showing = false;
    streaming_enter_fullscreen();
}

static void session_error(streaming_controller_t *controller) {
    static const char *btn_texts[] = {translatable("OK"), ""};
    lv_obj_t *dialog = lv_msgbox_create_i18n(NULL, locstr("Failed to start streaming"), streaming_errmsg,
                                             btn_texts, false);
    lv_obj_add_event_cb(dialog, session_error_dialog_cb, LV_EVENT_VALUE_CHANGED, controller);
    lv_obj_center(dialog);
}

static void session_error_dialog_cb(lv_event_t *event) {
    streaming_controller_t *controller = lv_event_get_user_data(event);
    lv_obj_t *dialog = lv_event_get_current_target(event);
    lv_msgbox_close_async(dialog);
    lv_fragment_del((lv_fragment_t *) controller);
}

static void overlay_key_cb(lv_event_t *e) {
    streaming_controller_t *controller = lv_event_get_user_data(e);
    lv_group_t *group = controller->group;
    switch (lv_event_get_key(e)) {
        case LV_KEY_LEFT:
            lv_group_focus_prev(group);
            break;
        case LV_KEY_RIGHT:
            lv_group_focus_next(group);
            break;
        default:
            break;
    }
}

static void update_buttons_layout(streaming_controller_t *controller) {
    lv_area_t coords;
    lv_obj_get_coords(controller->quit_btn, &coords);
    lv_area_center(&coords, &controller->button_points[1]);
    lv_obj_get_coords(controller->suspend_btn, &coords);
    lv_area_center(&coords, &controller->button_points[3]);
    lv_obj_get_coords(controller->kbd_btn, &coords);
    lv_area_center(&coords, &controller->button_points[4]);
    app_input_set_button_points(controller->button_points);
}