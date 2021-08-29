#include "manager.h"
#include "launcher/window.h"
#include "settings/settings.controller.h"

#include <memory.h>
#include <SDL.h>

typedef struct UIMANAGER_STACK {
    UIMANAGER_CONTROLLER_CREATOR creator;
    ui_view_controller_t *_controller;
    lv_obj_t *_view;
    struct UIMANAGER_STACK *prev;
    struct UIMANAGER_STACK *next;
} UIMANAGER_STACK, *PUIMANAGER_STACK;

#define LINKEDLIST_IMPL

#define LINKEDLIST_TYPE UIMANAGER_STACK
#define LINKEDLIST_PREFIX uistack
#define LINKEDLIST_DOUBLE 1

#include "util/linked_list.h"

#undef LINKEDLIST_DOUBLE
#undef LINKEDLIST_TYPE
#undef LINKEDLIST_PREFIX


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
    uimanager_t *instance = (uimanager_t *) ctx;
    PUIMANAGER_STACK top = instance->top;
    while (top) {
        item_destroy_view(top);
        top->_controller->destroy_controller(top->_controller);
        struct UIMANAGER_STACK *prev = top->prev;
        free(top);
        top = prev;
    }
    free(instance);
}

void uimanager_push(uimanager_ctx *ctx, UIMANAGER_CONTROLLER_CREATOR creator, const void *args) {
    uimanager_t *manager = (uimanager_t *) ctx;
    if (manager->top) {
        item_destroy_view(manager->top);
    }
    PUIMANAGER_STACK item = uistack_new();
    item->creator = creator;
    lv_obj_t *parent = manager->parent;
    item_create_view(manager, item, parent, args);
    uistack_append(manager->top, item);
    manager->top = item;
}

void uimanager_pop(uimanager_ctx *ctx) {
    uimanager_t *manager = (uimanager_t *) ctx;
    PUIMANAGER_STACK top = manager->top;
    if (!top) return;

    item_destroy_view(top);
    top->_controller->destroy_controller(top->_controller);
    top->_controller = NULL;
    PUIMANAGER_STACK prev = top->prev;
    free(top);
    if (prev) {
        prev->next = NULL;
        item_create_view(manager, prev, manager->parent, NULL);
    }
    manager->top = prev;
}

static void item_create_view(uimanager_t *manager, PUIMANAGER_STACK item, lv_obj_t *parent, const void *args) {
    ui_view_controller_t *controller = item->_controller;
    if (!controller) {
        controller = item->_controller = item->creator(args);
        controller->manager = manager;
    }
    lv_obj_t *view = controller->create_view(controller, parent);
    item->_view = controller->view = view;
    if (controller->view_created) {
        controller->view_created(controller, view);
    }
    lv_obj_add_event_cb(view, view_cb_delete, LV_EVENT_DELETE, controller);
}

static void item_destroy_view(PUIMANAGER_STACK item) {
    lv_obj_t *view = item->_view;
    lv_obj_del(view);
    item->_view = item->_controller->view = NULL;
}

void uimanager_push_from_event(lv_event_t *event) {
    uimanager_push(lv_scr_act(), event->user_data, NULL);
}

static void view_cb_delete(lv_event_t *event) {
    ui_view_controller_t *controller = event->user_data;
    if (!controller->destroy_view) return;
    controller->destroy_view(controller, event->target);
}