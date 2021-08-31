#include <util/user_event.h>
#include "streaming.controller.h"
#include "overlay.h"

static void exit_streaming(lv_event_t *event);

static void suspend_streaming(lv_event_t *event);

static void on_view_created(ui_view_controller_t *self, lv_obj_t *view);

static bool on_event(ui_view_controller_t *, int, void *, void *);

ui_view_controller_t *streaming_controller(void *args) {
    streaming_controller_t *controller = malloc(sizeof(streaming_controller_t));
    lv_memset_00(controller, sizeof(streaming_controller_t));
    controller->base.create_view = streaming_scene_create;
    controller->base.view_created = on_view_created;
    controller->base.dispatch_event = on_event;
    controller->base.destroy_controller = free;

    const STREAMING_SCENE_ARGS *req = (STREAMING_SCENE_ARGS *) args;
    streaming_begin(req->server, req->app);

    streaming_overlay_init();

    return (ui_view_controller_t *) controller;
}

static bool on_event(ui_view_controller_t *self, int which, void *data1, void *data2) {
    streaming_controller_t *controller = (streaming_controller_t *) self;
    switch (which) {
        case USER_STREAM_OPEN: {
            lv_obj_add_flag(controller->progress, LV_OBJ_FLAG_HIDDEN);
            break;
        }
        case USER_STREAM_CLOSE: {
            lv_obj_clear_flag(controller->progress, LV_OBJ_FLAG_HIDDEN);
            break;
        }
        case USER_STREAM_FINISHED: {
            uimanager_pop(controller->base.manager);
            break;
        }
        case USER_ST_QUITAPP_CONFIRM: {
            streaming_overlay_show();
            break;
        }
        default: {
            break;
        }
    }
    return false;
}

static void on_view_created(ui_view_controller_t *self, lv_obj_t *view) {
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
