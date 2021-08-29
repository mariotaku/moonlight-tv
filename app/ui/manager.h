#pragma once

#include "lvgl.h"

typedef void uimanager_ctx;

struct ui_view_controller_t {
    lv_obj_t *(*create_view)(struct ui_view_controller_t *controller, lv_obj_t *parent);

    bool (*dispatch_message)(struct ui_view_controller_t *controller, int which, void *data1, void *data2);

    void (*view_created)(struct ui_view_controller_t *controller, lv_obj_t *view);
    void (*destroy_view)(struct ui_view_controller_t *controller, lv_obj_t *view);

    void (*destroy_controller)(struct ui_view_controller_t *controller);

    uimanager_ctx *manager;
    lv_obj_t *view;
};

typedef struct ui_view_controller_t ui_view_controller_t;

typedef ui_view_controller_t *(*UIMANAGER_CONTROLLER_CREATOR)(const void *args);

uimanager_ctx *uimanager_new(lv_obj_t *parent);

void uimanager_destroy(uimanager_ctx *ctx);

void uimanager_pop(uimanager_ctx *ctx);

void uimanager_push(uimanager_ctx *ctx, UIMANAGER_CONTROLLER_CREATOR creator, const void *args);

void uimanager_push_from_event(lv_event_t *event);
