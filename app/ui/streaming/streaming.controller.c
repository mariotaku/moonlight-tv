#include <util/user_event.h>
#include "streaming.controller.h"
#include "overlay.h"

static bool on_event(streaming_controller_t *, int, void *, void *);

ui_view_controller_t *streaming_controller(const void *args) {
    streaming_controller_t *controller = malloc(sizeof(streaming_controller_t));
    lv_memset_00(controller, sizeof(streaming_controller_t));
    controller->base.create_view = streaming_scene_create;
    controller->base.dispatch_event = on_event;
    controller->base.destroy_controller = free;

    const STREAMING_SCENE_ARGS *req = (STREAMING_SCENE_ARGS *) args;
    streaming_begin(req->server, req->app);

    return (ui_view_controller_t *) controller;
}

static bool on_event(streaming_controller_t *controller, int which, void *data1, void *data2) {
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
    }
    return false;
}