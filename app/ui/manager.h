#pragma once

#include "lvgl.h"

typedef void uimanager_ctx;

struct ui_view_controller_t {
    lv_obj_t *(*create_view)(struct ui_view_controller_t *self, lv_obj_t *parent);

    void (*view_created)(struct ui_view_controller_t *self, lv_obj_t *view);

    void (*destroy_view)(struct ui_view_controller_t *self, lv_obj_t *view);

    void (*destroy_controller)(struct ui_view_controller_t *self);

    bool (*dispatch_event)(struct ui_view_controller_t *self, int which, void *data1, void *data2);

    uimanager_ctx *manager;
    lv_obj_t *view;
};

typedef struct ui_view_controller_t ui_view_controller_t;

typedef ui_view_controller_t *(*uimanager_controller_ctor_t)(void *args);

uimanager_ctx *uimanager_new(lv_obj_t *parent);

void uimanager_destroy(uimanager_ctx *ctx);

void uimanager_pop(uimanager_ctx *ctx);

void uimanager_push(uimanager_ctx *ctx, uimanager_controller_ctor_t creator, const void *args);

void uimanager_replace(uimanager_ctx *ctx, uimanager_controller_ctor_t creator, const void *args);

bool uimanager_dispatch_event(uimanager_ctx *ctx, int which, void *data1, void *data2);