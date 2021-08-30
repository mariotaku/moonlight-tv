#include "manager.h"

#include <assert.h>
#include <stdlib.h>

typedef struct UIMANAGER_STACK {
    uimanager_controller_ctor_t creator;
    ui_view_controller_t *controller;
    lv_obj_t *view;
    struct UIMANAGER_STACK *prev;
} UIMANAGER_STACK, *PUIMANAGER_STACK;

typedef struct {
    lv_obj_t *parent;
    PUIMANAGER_STACK top;
} uimanager_t;

static void item_create_view(uimanager_t *manager, PUIMANAGER_STACK item, lv_obj_t *parent, const void *args);

static void item_destroy_view(PUIMANAGER_STACK item);

static void view_cb_delete(lv_event_t *event);

uimanager_ctx *uimanager_new(lv_obj_t *parent) {
    assert(parent);
    uimanager_t *instance = malloc(sizeof(uimanager_t));
    instance->parent = parent;
    instance->top = NULL;
    return instance;
}

void uimanager_destroy(uimanager_ctx *ctx) {
    assert(ctx);
    uimanager_t *instance = (uimanager_t *) ctx;
    PUIMANAGER_STACK top = instance->top;
    while (top) {
        item_destroy_view(top);
        top->controller->destroy_controller(top->controller);
        struct UIMANAGER_STACK *prev = top->prev;
        free(top);
        top = prev;
    }
    free(instance);
}

void uimanager_push(uimanager_ctx *ctx, uimanager_controller_ctor_t creator, const void *args) {
    assert(ctx);
    assert(creator);
    uimanager_t *manager = (uimanager_t *) ctx;
    if (manager->top) {
        item_destroy_view(manager->top);
    }
    PUIMANAGER_STACK item = malloc(sizeof(UIMANAGER_STACK));
    item->creator = creator;
    item->controller = NULL;
    item->view = NULL;
    lv_obj_t *parent = manager->parent;
    item_create_view(manager, item, parent, args);
    PUIMANAGER_STACK top = manager->top;
    item->prev = top;
    manager->top = item;
}

void uimanager_replace(uimanager_ctx *ctx, uimanager_controller_ctor_t creator, const void *args) {
    assert(ctx);
    assert(creator);
    uimanager_t *manager = (uimanager_t *) ctx;
    PUIMANAGER_STACK top = manager->top;
    if (top) {
        item_destroy_view(top);
        top->controller->destroy_controller(top->controller);
    } else {
        top = manager->top = malloc(sizeof(UIMANAGER_STACK));
    }
    top->controller = NULL;
    top->creator = creator;
    item_create_view(manager, top, manager->parent, args);
}

void uimanager_pop(uimanager_ctx *ctx) {
    assert(ctx);
    uimanager_t *manager = (uimanager_t *) ctx;
    PUIMANAGER_STACK top = manager->top;
    if (!top) return;

    item_destroy_view(top);
    top->controller->destroy_controller(top->controller);
    top->controller = NULL;
    PUIMANAGER_STACK prev = top->prev;
    free(top);
    if (prev) {
        item_create_view(manager, prev, manager->parent, NULL);
    }
    manager->top = prev;
}

bool uimanager_dispatch_event(uimanager_ctx *ctx, int which, void *data1, void *data2) {
    assert(ctx);
    uimanager_t *manager = (uimanager_t *) ctx;
    PUIMANAGER_STACK top = manager->top;
    if (!top || !top->view) return false;
    ui_view_controller_t *controller = top->controller;
    if (!controller || !controller->dispatch_event) return false;
    return controller->dispatch_event(controller, which, data1, data2);
}

static void item_create_view(uimanager_t *manager, PUIMANAGER_STACK item, lv_obj_t *parent, const void *args) {
    ui_view_controller_t *controller = item->controller;
    if (!controller) {
        controller = item->controller = item->creator(args);
        controller->manager = manager;
    }
    lv_obj_t *view = controller->create_view(controller, parent);
    item->view = controller->view = view;
    if (controller->view_created) {
        controller->view_created(controller, view);
    }
    lv_obj_add_event_cb(view, view_cb_delete, LV_EVENT_DELETE, controller);
}

static void item_destroy_view(PUIMANAGER_STACK item) {
    lv_obj_t *view = item->view;
    lv_obj_del(view);
    item->view = item->controller->view = NULL;
}

static void view_cb_delete(lv_event_t *event) {
    ui_view_controller_t *controller = event->user_data;
    if (!controller->destroy_view) return;
    controller->destroy_view(controller, event->target);
}