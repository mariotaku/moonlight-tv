#include <util/user_event.h>
#include "streaming.controller.h"
#include "overlay.h"

static void exit_streaming(lv_event_t *event);

static void suspend_streaming(lv_event_t *event);

static void on_view_created(lv_obj_controller_t *self, lv_obj_t *view);

static bool on_event(lv_obj_controller_t *, int, void *, void *);

static void streaming_controller_ctor(lv_obj_controller_t *self, void *args);

const lv_obj_controller_class_t streaming_controller_class = {
        .constructor_cb = streaming_controller_ctor,
        .create_obj_cb = streaming_scene_create,
        .obj_created_cb = on_view_created,
        .event_cb = on_event,
        .instance_size = sizeof(streaming_controller_t),
};

static void streaming_controller_ctor(lv_obj_controller_t *self, void *args) {
    streaming_controller_t *controller = (streaming_controller_t *) self;

    const STREAMING_SCENE_ARGS *req = (STREAMING_SCENE_ARGS *) args;
    streaming_begin(req->server, req->app);

    streaming_overlay_init();
}

static bool on_event(lv_obj_controller_t *self, int which, void *data1, void *data2) {
    streaming_controller_t *controller = (streaming_controller_t *) self;
    switch (which) {
        case USER_STREAM_OPEN: {
            lv_obj_add_flag(controller->progress, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(controller->quit_btn, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(controller->suspend_btn, LV_OBJ_FLAG_HIDDEN);
            break;
        }
        case USER_STREAM_CLOSE: {
            lv_obj_clear_flag(controller->progress, LV_OBJ_FLAG_HIDDEN);
            break;
        }
        case USER_STREAM_FINISHED: {
            lv_controller_manager_pop(controller->base.manager);
            break;
        }
        case USER_ST_QUITAPP_CONFIRM: {
            lv_obj_clear_flag(controller->quit_btn, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(controller->suspend_btn, LV_OBJ_FLAG_HIDDEN);
            streaming_overlay_show();
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
}

static void exit_streaming(lv_event_t *event) {
    streaming_interrupt(true);
}

static void suspend_streaming(lv_event_t *event) {
    streaming_interrupt(false);
}
