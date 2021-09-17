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
    lv_obj_t *obj;
    bool obj_created;
    bool is_dialog;
    struct manager_stack_t *prev;
} manager_stack_t;

struct lv_controller_manager_t {
    lv_obj_t *container;
    manager_stack_t *top;
};

/**********************
 *  STATIC PROTOTYPES
 **********************/

static manager_stack_t *item_new(const lv_obj_controller_class_t *cls);

static lv_obj_controller_t *item_create_controller(lv_controller_manager_t *manager, manager_stack_t *item, void *args);

static void item_create_view(lv_controller_manager_t *manager, manager_stack_t *item, lv_obj_t *parent,
                             const lv_obj_class_t *check_type);

static void item_destroy_view(lv_controller_manager_t *manager, manager_stack_t *item);

static void item_destroy_controller(manager_stack_t *item);

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

lv_controller_manager_t *lv_controller_manager_create(lv_obj_t *container) {
    LV_ASSERT(container);
    lv_controller_manager_t *instance = lv_mem_alloc(sizeof(lv_controller_manager_t));
    instance->container = container;
    instance->top = NULL;
    return instance;
}

void lv_controller_manager_del(lv_controller_manager_t *manager) {
    LV_ASSERT(manager);
    manager_stack_t *top = manager->top;
    while (top) {
        LV_ASSERT(top->cls);
        item_destroy_view(manager, top);
        item_destroy_controller(top);
        struct manager_stack_t *prev = top->prev;
        lv_mem_free(top);
        top = prev;
    }
    lv_mem_free(manager);
}

void lv_controller_manager_push(lv_controller_manager_t *manager, const lv_obj_controller_class_t *cls, void *args) {
    LV_ASSERT(manager);
    LV_ASSERT(cls);
    manager_stack_t *item = item_new(cls);
    lv_obj_t *parent = manager->container;
    item_create_controller(manager, item, args);
    /* Destroy view of previous screen */
    if (manager->top) {
        item_destroy_view(manager, manager->top);
    }
    item_create_view(manager, item, parent, NULL);
    manager_stack_t *top = manager->top;
    item->prev = top;
    manager->top = item;
}

void lv_controller_manager_replace(lv_controller_manager_t *manager, const lv_obj_controller_class_t *cls, void *args) {
    LV_ASSERT(manager);
    LV_ASSERT(cls);
    manager_stack_t *top = item_new(cls);
    item_create_controller(manager, top, args);
    manager_stack_t *old = manager->top;
    if (old) {
        item_destroy_view(manager, old);
        item_destroy_controller(old);
        lv_mem_free(old);
    }
    manager->top = top;
    item_create_view(manager, top, manager->container, NULL);
}

void lv_controller_manager_show(lv_controller_manager_t *manager, const lv_obj_controller_class_t *cls, void *args) {
    LV_ASSERT(manager);
    LV_ASSERT(cls);
    manager_stack_t *item = item_new(cls);
    item_create_controller(manager, item, args);
    item_create_view(manager, item, NULL, &lv_dialog_class);
    item->is_dialog = true;
    manager_stack_t *top = manager->top;
    item->prev = top;
    manager->top = item;
}

void lv_controller_manager_pop(lv_controller_manager_t *manager) {
    LV_ASSERT(manager);
    manager_stack_t *top = manager->top;
    if (!top) return;
    LV_ASSERT(!top->is_dialog);
    if (top->is_dialog) return;
    manager_stack_t *prev = top->prev;
    if (prev) {
        item_create_controller(manager, prev, NULL);
    }
    item_destroy_view(manager, top);
    item_destroy_controller(top);
    lv_mem_free(top);
    if (prev) {
        item_create_view(manager, prev, manager->container, NULL);
    }
    manager->top = prev;
}

bool lv_controller_manager_dispatch_event(lv_controller_manager_t *manager, int which, void *data1, void *data2) {
    LV_ASSERT(manager);
    manager_stack_t *top = manager->top;
    if (!top) return false;
    lv_obj_controller_t *controller = top->controller;
    if (!controller || !top->cls->event_cb) return false;
    return top->cls->event_cb(controller, which, data1, data2);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static manager_stack_t *item_new(const lv_obj_controller_class_t *cls) {
    LV_ASSERT(cls->instance_size);
    manager_stack_t *item = lv_mem_alloc(sizeof(manager_stack_t));
    lv_memset_00(item, sizeof(manager_stack_t));
    item->cls = cls;
    return item;
}

static lv_obj_controller_t *item_create_controller(lv_controller_manager_t *manager, manager_stack_t *item,
                                                   void *args) {
    if (item->controller) return item->controller;
    const lv_obj_controller_class_t *cls = item->cls;
    LV_ASSERT(cls->instance_size);
    lv_obj_controller_t *controller = lv_mem_alloc(cls->instance_size);
    lv_memset_00(controller, cls->instance_size);
    controller->cls = cls;
    controller->manager = manager;
    if (cls->constructor_cb) {
        cls->constructor_cb(controller, args);
    }
    item->controller = controller;
    return controller;
}

static void item_create_view(lv_controller_manager_t *manager, manager_stack_t *item, lv_obj_t *parent,
                             const lv_obj_class_t *check_type) {
    LV_ASSERT(item->controller);
    const lv_obj_controller_class_t *cls = item->cls;
    LV_ASSERT(cls->create_obj_cb);
    lv_obj_controller_t *controller = item->controller;
    lv_obj_t *view = cls->create_obj_cb(controller, parent);
    if (check_type) {
        LV_ASSERT(lv_obj_has_class(view, check_type));
    }
    item->obj = controller->obj = view;
    item->obj_created = true;
    if (cls->obj_created_cb) {
        cls->obj_created_cb(controller, view);
    }
    if (view) {
        lv_obj_add_event_cb(view, view_cb_delete, LV_EVENT_DELETE, controller);
    }
}

static void item_destroy_view(lv_controller_manager_t *manager, manager_stack_t *item) {
    if (!item->obj_created) return;
    lv_obj_controller_t *controller = item->controller;
    if (item->obj) {
        lv_obj_del(item->obj);
    } else {
        lv_obj_clean(manager->container);
        if (item->cls->obj_deleted_cb) {
            item->cls->obj_deleted_cb(controller, NULL);
        }
    }
    item->obj_created = false;
    item->obj = controller->obj = NULL;
}

static void item_destroy_controller(manager_stack_t *item) {
    const lv_obj_controller_class_t *cls = item->cls;
    if (cls->destructor_cb) {
        cls->destructor_cb(item->controller);
    }
    lv_mem_free(item->controller);
    item->controller = NULL;
}

static void view_cb_delete(lv_event_t *event) {
    lv_obj_controller_t *controller = event->user_data;
    const lv_obj_controller_class_t *cls = controller->cls;
    if (!cls->obj_deleted_cb || event->target != controller->obj) return;
    cls->obj_deleted_cb(controller, event->target);
}
