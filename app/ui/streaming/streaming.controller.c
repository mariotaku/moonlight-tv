#include <util/user_event.h>
#include <ui/root.h>
#include "streaming.controller.h"
#include "streaming.view.h"


static void exit_streaming(lv_event_t *event);

static void suspend_streaming(lv_event_t *event);

static void hide_overlay(lv_event_t *event);

static bool show_overlay(streaming_controller_t *controller);

static void on_view_created(lv_obj_controller_t *self, lv_obj_t *view);

static bool on_event(lv_obj_controller_t *, int, void *, void *);

static void streaming_controller_ctor(lv_obj_controller_t *self, void *args);

static void session_error(streaming_controller_t *controller);

static void session_error_dialog_cb(lv_event_t *event);

const lv_obj_controller_class_t streaming_controller_class = {
        .constructor_cb = streaming_controller_ctor,
        .create_obj_cb = streaming_scene_create,
        .obj_created_cb = on_view_created,
        .event_cb = on_event,
        .instance_size = sizeof(streaming_controller_t),
};

static bool stream_overlay_showing;

bool streaming_overlay_shown() {
    return stream_overlay_showing;
}

static void streaming_controller_ctor(lv_obj_controller_t *self, void *args) {
    streaming_controller_t *controller = (streaming_controller_t *) self;

    const streaming_scene_arg_t *req = (streaming_scene_arg_t *) args;
    streaming_begin(req->server, req->app);

    stream_overlay_showing = false;
}

static bool on_event(lv_obj_controller_t *self, int which, void *data1, void *data2) {
    streaming_controller_t *controller = (streaming_controller_t *) self;
    switch (which) {
        case USER_STREAM_CONNECTING: {
            lv_obj_clear_flag(controller->progress, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(controller->progress_label, "Starting session.");
            lv_obj_add_flag(controller->quit_btn, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(controller->suspend_btn, LV_OBJ_FLAG_HIDDEN);
            break;
        }
        case USER_STREAM_OPEN: {
            lv_obj_add_flag(controller->progress, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(controller->quit_btn, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(controller->suspend_btn, LV_OBJ_FLAG_HIDDEN);
            break;
        }
        case USER_STREAM_CLOSE: {
            lv_obj_clear_flag(controller->progress, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(controller->progress_label, "Disconnecting.");
            lv_obj_add_flag(controller->quit_btn, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(controller->suspend_btn, LV_OBJ_FLAG_HIDDEN);
            break;
        }
        case USER_STREAM_FINISHED: {
            lv_obj_add_flag(controller->progress, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(controller->quit_btn, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(controller->suspend_btn, LV_OBJ_FLAG_HIDDEN);
            if (streaming_errno != 0) {
                session_error(controller);
                break;
            }
            lv_obj_controller_pop((lv_obj_controller_t *) controller);
            break;
        }
        case USER_ST_QUITAPP_CONFIRM: {
            show_overlay(controller);
            break;
        }
        default: {
            break;
        }
    }
    return false;
}

static void on_view_created(lv_obj_controller_t *self, lv_obj_t *view) {
    streaming_controller_t *controller = (streaming_controller_t *) self;
    lv_obj_add_event_cb(controller->quit_btn, exit_streaming, LV_EVENT_CLICKED, self);
    lv_obj_add_event_cb(controller->suspend_btn, suspend_streaming, LV_EVENT_CLICKED, self);
    lv_obj_add_event_cb(controller->base.obj, hide_overlay, LV_EVENT_CLICKED, self);
}

static void exit_streaming(lv_event_t *event) {
    streaming_interrupt(true);
}

static void suspend_streaming(lv_event_t *event) {
    streaming_interrupt(false);
}

bool show_overlay(streaming_controller_t *controller) {
    if (stream_overlay_showing)
        return false;
    stream_overlay_showing = true;
    lv_obj_clear_flag(controller->quit_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(controller->suspend_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(controller->base.obj, LV_OBJ_FLAG_CLICKABLE);

    streaming_enter_overlay(0, 0, ui_display_width / 2, ui_display_height / 2);
    return true;
}

static void hide_overlay(lv_event_t *event) {
    streaming_controller_t *controller = (streaming_controller_t *) lv_event_get_user_data(event);
    lv_obj_clear_flag(controller->base.obj, LV_OBJ_FLAG_CLICKABLE);
    if (!stream_overlay_showing)
        return;
    stream_overlay_showing = false;
    streaming_enter_fullscreen();
}

static void session_error(streaming_controller_t *controller) {
    static const char *btn_texts[] = {"OK", ""};
    lv_obj_t *dialog = lv_msgbox_create(NULL, "Failed to start session", streaming_errmsg, btn_texts,
                                        false);
    lv_obj_add_event_cb(dialog, session_error_dialog_cb, LV_EVENT_VALUE_CHANGED, controller);
    lv_obj_center(dialog);
}

static void session_error_dialog_cb(lv_event_t *event) {
    streaming_controller_t *controller = lv_event_get_user_data(event);
    lv_obj_t *dialog = lv_event_get_current_target(event);
    lv_msgbox_close_async(dialog);
    lv_obj_controller_pop((lv_obj_controller_t *) controller);
}