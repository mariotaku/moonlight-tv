/**
 * @file lv_obj_controller.h
 *
 */

#ifndef LV_OBJ_CONTROLLER_H
#define LV_OBJ_CONTROLLER_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "lvgl.h"


/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef struct lv_controller_manager_t lv_controller_manager_t;

typedef struct lv_obj_controller_t lv_obj_controller_t;
typedef struct lv_obj_controller_class_t lv_obj_controller_class_t;

struct lv_obj_controller_t {
    const lv_obj_controller_class_t *cls;
    lv_controller_manager_t *manager;
    lv_obj_t *obj;
};

struct lv_obj_controller_class_t {
    void (*constructor_cb)(struct lv_obj_controller_t *self, void *args);

    void (*destructor_cb)(struct lv_obj_controller_t *self);

    lv_obj_t *(*create_obj_cb)(struct lv_obj_controller_t *self, lv_obj_t *parent);

    void (*obj_created_cb)(struct lv_obj_controller_t *self, lv_obj_t *view);

    void (*obj_deleted_cb)(struct lv_obj_controller_t *self, lv_obj_t *view);

    bool (*event_cb)(struct lv_obj_controller_t *self, int which, void *data1, void *data2);

    size_t instance_size;
};

/**********************
 * GLOBAL PROTOTYPES
 **********************/

lv_controller_manager_t *lv_controller_manager_create(lv_obj_t *parent);

void lv_controller_manager_del(lv_controller_manager_t *manager);

void lv_controller_manager_pop(lv_controller_manager_t *manager);

void lv_controller_manager_push(lv_controller_manager_t *manager, const lv_obj_controller_class_t *cls, void *args);

void lv_controller_manager_replace(lv_controller_manager_t *manager, const lv_obj_controller_class_t *cls, void *args);

bool lv_controller_manager_dispatch_event(lv_controller_manager_t *manager, int which, void *data1, void *data2);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_OBJ_CONTROLLER_H*/