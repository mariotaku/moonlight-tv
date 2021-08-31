//
// Created by Mariotaku on 2021/08/31.
//

#include <malloc.h>
#include <backend/application_manager.h>
#include "apps.controller.h"

typedef struct {
    ui_view_controller_t base;
    APPMANAGER_CALLBACKS _appmanager_callbacks;
    PSERVER_LIST server;
    lv_obj_t *applist, *appload;
} apps_controller_t;

static lv_obj_t *apps_view(ui_view_controller_t *self, lv_obj_t *parent);

static void on_view_created(ui_view_controller_t *self, lv_obj_t *view);

static void on_destroy_view(ui_view_controller_t *self, lv_obj_t *view);

static void on_apps_updated(void *userdata, PSERVER_LIST node);

void launcher_open_game(lv_event_t *event);

static void update_data(apps_controller_t *controller, PSERVER_LIST node);

ui_view_controller_t *apps_controller(void *args) {
    apps_controller_t *controller = malloc(sizeof(apps_controller_t));
    memset(controller, 0, sizeof(apps_controller_t));
    controller->base.create_view = apps_view;
    controller->base.view_created = on_view_created;
    controller->base.destroy_view = on_destroy_view;
    controller->base.destroy_controller = (void (*)(ui_view_controller_t *)) free;
    controller->_appmanager_callbacks.updated = on_apps_updated;
    controller->_appmanager_callbacks.userdata = controller;
    controller->server = args;
    return (ui_view_controller_t *) controller;
}

static lv_obj_t *apps_view(ui_view_controller_t *self, lv_obj_t *parent) {
    apps_controller_t *controller = (apps_controller_t *) self;

    lv_obj_t *applist = lv_list_create(parent);
    lv_obj_set_style_pad_all(applist, 0, 0);
    lv_obj_set_style_radius(applist, 0, 0);
    lv_obj_set_style_border_side(applist, LV_BORDER_SIDE_NONE, 0);
    lv_obj_set_style_bg_opa(applist, 0, 0);
    lv_obj_set_size(applist, LV_PCT(100), LV_PCT(100));
    lv_obj_t *appload = lv_spinner_create(parent, 1000, 60);
    lv_obj_set_size(appload, lv_dpx(60), lv_dpx(60));
    lv_obj_center(appload);

    controller->applist = applist;
    controller->appload = appload;
    return NULL;
}

static void on_view_created(ui_view_controller_t *self, lv_obj_t *view) {
    apps_controller_t *controller = (apps_controller_t *) self;
    appmanager_register_callbacks(&controller->_appmanager_callbacks);

    update_data(controller, controller->server);
    if (!controller->server->apps) {
        application_manager_load(controller->server);
    }
}

static void on_destroy_view(ui_view_controller_t *self, lv_obj_t *view) {
    apps_controller_t *controller = (apps_controller_t *) self;
    appmanager_unregister_callbacks(&controller->_appmanager_callbacks);
}

static void on_apps_updated(void *userdata, PSERVER_LIST node) {
    apps_controller_t *controller = (apps_controller_t *) userdata;
    if (node != controller->server) return;
    update_data(controller, node);
}

static void update_data(apps_controller_t *controller, PSERVER_LIST node) {
    lv_obj_t *applist = controller->applist;
    lv_obj_t *appload = controller->appload;
    lv_obj_clean(applist);
    if (node->state.code == SERVER_STATE_ONLINE) {
        if (node->appload) {
            lv_obj_add_flag(applist, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(appload, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_clear_flag(applist, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(appload, LV_OBJ_FLAG_HIDDEN);
            for (PAPP_DLIST cur = node->apps; cur != NULL; cur = cur->next) {
                lv_obj_t *item = lv_list_add_btn(applist, NULL, cur->name);
                lv_obj_set_style_bg_opa(item, LV_STATE_DEFAULT, 0);
                item->user_data = cur;
                lv_obj_add_event_cb(item, launcher_open_game, LV_EVENT_CLICKED, controller);
            }
        }
    } else {
        lv_obj_add_flag(applist, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(appload, LV_OBJ_FLAG_HIDDEN);
    }
}

