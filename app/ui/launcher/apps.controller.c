//
// Created by Mariotaku on 2021/08/31.
//

#include "app.h"

#include <malloc.h>
#include "backend/application_manager.h"
#include "ui/streaming/overlay.h"
#include "ui/streaming/streaming.controller.h"
#include "apps.controller.h"

typedef struct {
    ui_view_controller_t base;
    PCMANAGER_CALLBACKS _pcmanager_callbacks;
    APPMANAGER_CALLBACKS _appmanager_callbacks;
    lv_group_t *group;
    PSERVER_LIST node;
    lv_obj_t *applist, *appload;
} apps_controller_t;

static lv_obj_t *apps_view(ui_view_controller_t *self, lv_obj_t *parent);

static void on_view_created(ui_view_controller_t *self, lv_obj_t *view);

static void on_destroy_view(ui_view_controller_t *self, lv_obj_t *view);

static void on_host_updated(void *userdata, PPCMANAGER_RESP resp);

static void on_apps_updated(void *userdata, PSERVER_LIST node);

static void launcher_open_game(lv_event_t *event);

static void update_data(apps_controller_t *controller);

ui_view_controller_t *apps_controller(void *args) {
    apps_controller_t *controller = malloc(sizeof(apps_controller_t));
    memset(controller, 0, sizeof(apps_controller_t));
    controller->base.create_view = apps_view;
    controller->base.view_created = on_view_created;
    controller->base.destroy_view = on_destroy_view;
    controller->base.destroy_controller = (void (*)(ui_view_controller_t *)) free;
    controller->_pcmanager_callbacks.added = NULL;
    controller->_pcmanager_callbacks.updated = on_host_updated;
    controller->_pcmanager_callbacks.userdata = controller;
    controller->_appmanager_callbacks.updated = on_apps_updated;
    controller->_appmanager_callbacks.userdata = controller;
    controller->node = args;
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
    pcmanager_register_callbacks(&controller->_pcmanager_callbacks);
    lv_group_remove_obj(controller->applist);
    controller->group = lv_group_create();
    lv_indev_set_group(app_indev_key, controller->group);

    update_data(controller);
    if (!controller->node->apps) {
        application_manager_load(controller->node);
    }
}

static void on_destroy_view(ui_view_controller_t *self, lv_obj_t *view) {
    apps_controller_t *controller = (apps_controller_t *) self;
    lv_group_del(controller->group);
    appmanager_unregister_callbacks(&controller->_appmanager_callbacks);
    pcmanager_unregister_callbacks(&controller->_pcmanager_callbacks);

    lv_indev_set_group(app_indev_key, lv_group_get_default());
}

static void on_host_updated(void *userdata, PPCMANAGER_RESP resp) {
    apps_controller_t *controller = (apps_controller_t *) userdata;
    if (resp->server != controller->node->server) return;
    update_data(controller);
}

static void on_apps_updated(void *userdata, PSERVER_LIST node) {
    apps_controller_t *controller = (apps_controller_t *) userdata;
    if (node != controller->node) return;
    update_data(controller);
}

static void update_data(apps_controller_t *controller) {
    PSERVER_LIST node = controller->node;
    assert(node);
    lv_obj_t *applist = controller->applist;
    lv_obj_t *appload = controller->appload;
    if (node->state.code == SERVER_STATE_NONE) {
        lv_obj_add_flag(applist, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(appload, LV_OBJ_FLAG_HIDDEN);
    } else if (node->state.code == SERVER_STATE_ONLINE) {
        if (node->appload) {
            lv_obj_add_flag(applist, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(appload, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_clear_flag(applist, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(appload, LV_OBJ_FLAG_HIDDEN);
            if (lv_obj_get_child_cnt(applist) == 0) {
                for (PAPP_DLIST cur = node->apps; cur != NULL; cur = cur->next) {
                    lv_obj_t *item = lv_list_add_btn(applist, NULL, cur->name);
                    lv_group_remove_obj(item);
                    lv_group_add_obj(controller->group, item);
                    lv_obj_set_style_bg_opa(item, LV_STATE_DEFAULT, 0);
                    item->user_data = cur;
                    lv_obj_add_event_cb(item, launcher_open_game, LV_EVENT_CLICKED, controller);
                }
            }
        }
    } else {
        lv_obj_add_flag(applist, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(appload, LV_OBJ_FLAG_HIDDEN);
    }
}

static void launcher_open_game(lv_event_t *event) {
    apps_controller_t *controller = event->user_data;
    STREAMING_SCENE_ARGS args = {
            .server = controller->node->server,
            .app = (PAPP_DLIST) event->target->user_data
    };
    uimanager_push(app_uimanager, streaming_controller, &args);
}
