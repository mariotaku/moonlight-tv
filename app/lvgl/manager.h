#pragma once

#include "lvgl.h"

typedef void uimanager_ctx;

struct lv_obj_controller_t {
    lv_obj_t *(*create_view)(struct lv_obj_controller_t *self, lv_obj_t *parent);

    void (*view_created)(struct lv_obj_controller_t *self, lv_obj_t *view);

    void (*destroy_view)(struct lv_obj_controller_t *self, lv_obj_t *view);

    void (*destroy_controller)(struct lv_obj_controller_t *self);

    bool (*dispatch_event)(struct lv_obj_controller_t *self, int which, void *data1, void *data2);

    uimanager_ctx *manager;
    lv_obj_t *view;
};

typedef struct lv_obj_controller_t lv_obj_controller_t;

typedef lv_obj_controller_t *(*lv_obj_controller_ctor_t)(void *args);

uimanager_ctx *uimanager_new(lv_obj_t *parent);

void uimanager_destroy(uimanager_ctx *ctx);

void uimanager_pop(uimanager_ctx *ctx);

void uimanager_push(uimanager_ctx *ctx, lv_obj_controller_ctor_t creator, void *args);

void uimanager_replace(uimanager_ctx *ctx, lv_obj_controller_ctor_t creator, void *args);

bool uimanager_dispatch_event(uimanager_ctx *ctx, int which, void *data1, void *data2);

void ui_view_controller_free(lv_obj_controller_t *);