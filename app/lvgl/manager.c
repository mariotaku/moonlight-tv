#include "manager.h"

#include <assert.h>

typedef struct UIMANAGER_STACK {
    lv_obj_controller_ctor_t creator;
    lv_obj_controller_t *controller;
    lv_obj_t *view;
    struct UIMANAGER_STACK *prev;
} UIMANAGER_STACK, *PUIMANAGER_STACK;

typedef struct {
    lv_obj_t *parent;
    PUIMANAGER_STACK top;
} uimanager_t;

static void item_create_view(uimanager_t *manager, PUIMANAGER_STACK item, lv_obj_t *parent, void *args);

static void item_destroy_view(uimanager_t *manager, PUIMANAGER_STACK item);

static void view_cb_delete(lv_event_t *event);

uimanager_ctx *uimanager_new(lv_obj_t *parent) {
    assert(parent);
    uimanager_t *instance = lv_mem_alloc(sizeof(uimanager_t));
    instance->parent = parent;
    instance->top = NULL;
    return instance;
}

void uimanager_destroy(uimanager_ctx *ctx) {
    assert(ctx);
    uimanager_t *manager = (uimanager_t *) ctx;
    PUIMANAGER_STACK top = manager->top;
    while (top) {
        item_destroy_view(manager, top);
        top->controller->destroy_controller(top->controller);
        struct UIMANAGER_STACK *prev = top->prev;
        lv_mem_free(top);
        top = prev;
    }
    lv_mem_free(manager);
}

void uimanager_push(uimanager_ctx *ctx, lv_obj_controller_ctor_t creator, void *args) {
    assert(ctx);
    assert(creator);
    uimanager_t *manager = (uimanager_t *) ctx;
    if (manager->top) {
        item_destroy_view(manager, manager->top);
    }
    PUIMANAGER_STACK item = lv_mem_alloc(sizeof(UIMANAGER_STACK));
    lv_memset_00(item, sizeof(UIMANAGER_STACK));
    item->creator = creator;
    lv_obj_t *parent = manager->parent;
    item_create_view(manager, item, parent, args);
    PUIMANAGER_STACK top = manager->top;
    item->prev = top;
    manager->top = item;
}

void uimanager_replace(uimanager_ctx *ctx, lv_obj_controller_ctor_t creator, void *args) {
    assert(ctx);
    assert(creator);
    uimanager_t *manager = (uimanager_t *) ctx;
    PUIMANAGER_STACK top = manager->top;
    if (top) {
        item_destroy_view(manager, top);
        top->controller->destroy_controller(top->controller);
    } else {
        top = manager->top = lv_mem_alloc(sizeof(UIMANAGER_STACK));
        lv_memset_00(top, sizeof(UIMANAGER_STACK));
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

    item_destroy_view(manager, top);
    top->controller->destroy_controller(top->controller);
    top->controller = NULL;
    PUIMANAGER_STACK prev = top->prev;
    lv_mem_free(top);
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
    lv_obj_controller_t *controller = top->controller;
    if (!controller || !controller->dispatch_event) return false;
    return controller->dispatch_event(controller, which, data1, data2);
}

void ui_view_controller_free(lv_obj_controller_t *controller) {
    lv_mem_free(controller);
}

static void item_create_view(uimanager_t *manager, PUIMANAGER_STACK item, lv_obj_t *parent, void *args) {
    lv_obj_controller_t *controller = item->controller;
    if (!controller) {
        controller = item->controller = item->creator(args);
        controller->manager = manager;
    }
    lv_obj_t *view = controller->create_view(controller, parent);
    item->view = controller->view = view;
    if (controller->view_created) {
        controller->view_created(controller, view);
    }
    if (view) {
        lv_obj_add_event_cb(view, view_cb_delete, LV_EVENT_DELETE, controller);
    }
}

static void item_destroy_view(uimanager_t *manager, PUIMANAGER_STACK item) {
    lv_obj_t *view = item->view;
    lv_obj_controller_t *controller = item->controller;
    if (view) {
        lv_obj_del(view);
    } else {
        lv_obj_clean(manager->parent);
        if (controller->destroy_view) {
            controller->destroy_view(controller, NULL);
        }
    }
    item->view = controller->view = NULL;
}

static void view_cb_delete(lv_event_t *event) {
    lv_obj_controller_t *controller = event->user_data;
    if (!controller->destroy_view) return;
    controller->destroy_view(controller, event->target);
}
