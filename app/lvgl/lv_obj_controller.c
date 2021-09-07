/**
 * @file lv_obj_controller.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#include "lv_obj_controller.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef struct manager_stack_t {
    const lv_obj_controller_class_t *cls;
    lv_obj_controller_t *controller;
    lv_obj_t *view;
    struct manager_stack_t *prev;
} manager_stack_t;

struct lv_controller_manager_t {
    lv_obj_t *parent;
    manager_stack_t *top;
};

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void item_create_view(lv_controller_manager_t *manager, manager_stack_t *item, lv_obj_t *parent, void *args);

static void item_destroy_view(lv_controller_manager_t *manager, manager_stack_t *item);

static void view_cb_delete(lv_event_t *event);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_controller_manager_t *lv_controller_manager_create(lv_obj_t *parent) {
    LV_ASSERT(parent);
    lv_controller_manager_t *instance = lv_mem_alloc(sizeof(lv_controller_manager_t));
    instance->parent = parent;
    instance->top = NULL;
    return instance;
}

void lv_controller_manager_del(lv_controller_manager_t *manager) {
    LV_ASSERT(manager);
    manager_stack_t *top = manager->top;
    while (top) {
        item_destroy_view(manager, top);
        top->cls->destructor_cb(top->controller);
        struct manager_stack_t *prev = top->prev;
        lv_mem_free(top);
        top = prev;
    }
    lv_mem_free(manager);
}

void lv_controller_manager_push(lv_controller_manager_t *manager, const lv_obj_controller_class_t *cls, void *args) {
    LV_ASSERT(manager);
    LV_ASSERT(cls);
    if (manager->top) {
        item_destroy_view(manager, manager->top);
    }
    manager_stack_t *item = lv_mem_alloc(sizeof(manager_stack_t));
    lv_memset_00(item, sizeof(manager_stack_t));
    item->cls = cls;
    lv_obj_t *parent = manager->parent;
    item_create_view(manager, item, parent, args);
    manager_stack_t *top = manager->top;
    item->prev = top;
    manager->top = item;
}

void lv_controller_manager_replace(lv_controller_manager_t *manager, const lv_obj_controller_class_t *cls, void *args) {
    LV_ASSERT(manager);
    LV_ASSERT(cls);
    manager_stack_t *top = manager->top;
    if (top) {
        item_destroy_view(manager, top);
        top->cls->destructor_cb(top->controller);
    } else {
        top = manager->top = lv_mem_alloc(sizeof(manager_stack_t));
        lv_memset_00(top, sizeof(manager_stack_t));
    }
    top->controller = NULL;
    top->cls = cls;
    item_create_view(manager, top, manager->parent, args);
}

void lv_controller_manager_pop(lv_controller_manager_t *manager) {
    LV_ASSERT(manager);
    manager_stack_t *top = manager->top;
    if (!top) return;

    item_destroy_view(manager, top);
    top->cls->destructor_cb(top->controller);
    top->controller = NULL;
    manager_stack_t *prev = top->prev;
    lv_mem_free(top);
    if (prev) {
        item_create_view(manager, prev, manager->parent, NULL);
    }
    manager->top = prev;
}

bool lv_controller_manager_dispatch_event(lv_controller_manager_t *manager, int which, void *data1, void *data2) {
    LV_ASSERT(manager);
    manager_stack_t *top = manager->top;
    if (!top || !top->view) return false;
    lv_obj_controller_t *controller = top->controller;
    if (!controller || !top->cls->event_cb) return false;
    return top->cls->event_cb(controller, which, data1, data2);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void item_create_view(lv_controller_manager_t *manager, manager_stack_t *item, lv_obj_t *parent, void *args) {
    const lv_obj_controller_class_t *cls = item->cls;
    lv_obj_controller_t *controller = item->controller;
    if (!controller) {
        LV_ASSERT(cls->instance_size);
        controller = item->controller = lv_mem_alloc(cls->instance_size);
        lv_memset_00(controller, cls->instance_size);
        controller->cls = cls;
        controller->manager = manager;
        cls->constructor_cb(controller, args);
    }
    lv_obj_t *view = cls->create_obj_cb(controller, parent);
    item->view = controller->obj = view;
    if (cls->obj_created_cb) {
        cls->obj_created_cb(controller, view);
    }
    if (view) {
        lv_obj_add_event_cb(view, view_cb_delete, LV_EVENT_DELETE, controller);
    }
}

static void item_destroy_view(lv_controller_manager_t *manager, manager_stack_t *item) {
    lv_obj_t *view = item->view;
    lv_obj_controller_t *controller = item->controller;
    if (view) {
        lv_obj_del(view);
    } else {
        lv_obj_clean(manager->parent);
        if (item->cls->obj_deleted_cb) {
            item->cls->obj_deleted_cb(controller, NULL);
        }
    }
    item->view = controller->obj = NULL;
}

static void view_cb_delete(lv_event_t *event) {
    lv_obj_controller_t *controller = event->user_data;
    const lv_obj_controller_class_t *cls = controller->cls;
    if (!cls->obj_deleted_cb || event->target != controller->obj) return;
    cls->obj_deleted_cb(controller, event->target);
}
