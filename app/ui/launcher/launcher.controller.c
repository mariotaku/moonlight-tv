#include "ui/streaming/streaming.controller.h"
#include "lvgl.h"
#include "ui/streaming/overlay.h"
#include "window.h"

void launcher_view_init(launcher_controller_t *controller);

void launcher_view_destroy(launcher_controller_t *controller, lv_obj_t *view);

static void launcher_handle_server_updated(launcher_controller_t *controller, PPCMANAGER_RESP resp);

static void launcher_handle_apps_updated(launcher_controller_t *controller, PSERVER_LIST node);

static void update_selected(launcher_controller_t *controller, PSERVER_LIST node);

static void update_pclist(launcher_controller_t *controller);

static void cb_pc_selected(lv_event_t *event);

void launcher_open_game(lv_event_t *event);

void launcher_win_update_selected(launcher_controller_t *controller, PSERVER_LIST node);

ui_view_controller_t *launcher_controller(const void *args) {
    launcher_controller_t *controller = malloc(sizeof(launcher_controller_t));
    lv_memset_00(controller, sizeof(launcher_controller_t));
    controller->base.create_view = launcher_win_create;
    controller->base.destroy_controller = (void (*)(ui_view_controller_t *)) free;
    controller->base.view_created = launcher_view_init;
    controller->base.destroy_view = launcher_view_destroy;
    controller->_pcmanager_callbacks.added = launcher_handle_server_updated;
    controller->_pcmanager_callbacks.updated = launcher_handle_server_updated;
    controller->_pcmanager_callbacks.userdata = controller;
    controller->_appmanager_callbacks.updated = launcher_handle_apps_updated;
    controller->_appmanager_callbacks.userdata = controller;
    return (ui_view_controller_t *) controller;
}

void launcher_view_init(launcher_controller_t *controller) {
    pcmanager_register_callbacks(&controller->_pcmanager_callbacks);
    appmanager_register_callbacks(&controller->_appmanager_callbacks);
    update_pclist(controller);
}

void launcher_view_destroy(launcher_controller_t *controller, lv_obj_t *view) {
    pcmanager_unregister_callbacks(&controller->_pcmanager_callbacks);
    appmanager_unregister_callbacks(&controller->_appmanager_callbacks);
}

static void update_selected(launcher_controller_t *controller, PSERVER_LIST node) {
    if (node && node->state.code == SERVER_STATE_ONLINE && !node->apps) {
        application_manager_load(node);
    }
    controller->selected_server = node;
    launcher_win_update_selected(controller, node);
}

void launcher_handle_server_updated(launcher_controller_t *controller, PPCMANAGER_RESP resp) {
    update_pclist(controller);
//    update_selected(controller, controller->selected_server);
}

static void cb_pc_selected(lv_event_t *event) {
    launcher_controller_t *controller = event->user_data;
    update_selected(controller, (PSERVER_LIST) event->target->user_data);
}

void launcher_open_game(lv_event_t *event) {
    launcher_controller_t *controller = event->user_data;
    STREAMING_SCENE_ARGS args = {
            .server = controller->selected_server->server,
            .app = (PAPP_DLIST) event->target->user_data
    };
    uimanager_push(controller->base.manager, streaming_controller, &args);
}

void launcher_handle_apps_updated(launcher_controller_t *controller, PSERVER_LIST node) {
    PSERVER_LIST selected = controller->selected_server;
    if (node != selected) return;
    update_selected(controller, selected);
}

static void update_pclist(launcher_controller_t *controller) {
    lv_obj_clean(controller->pclist);
    for (PSERVER_LIST cur = computer_list; cur != NULL; cur = cur->next) {
        lv_obj_t *pcitem = lv_list_add_btn(controller->pclist, NULL, cur->server->hostname);
        pcitem->user_data = cur;
        lv_obj_add_event_cb(pcitem, cb_pc_selected, LV_EVENT_CLICKED, controller);
    }
}

void launcher_win_update_selected(launcher_controller_t *controller, PSERVER_LIST node) {
    lv_obj_t *applist = controller->applist;
    lv_obj_t *appload = controller->appload;
    lv_obj_clean(applist);
    if (!node) {
        lv_obj_add_flag(applist, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(appload, LV_OBJ_FLAG_HIDDEN);
    } else if (node->state.code == SERVER_STATE_ONLINE) {
        if (node->appload) {
            lv_obj_add_flag(applist, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(appload, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_clear_flag(applist, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(appload, LV_OBJ_FLAG_HIDDEN);
            for (PAPP_DLIST cur = node->apps; cur != NULL; cur = cur->next) {
                lv_obj_t *item = lv_list_add_btn(applist, NULL, cur->name);
                item->user_data = cur;
                lv_obj_add_event_cb(item, launcher_open_game, LV_EVENT_CLICKED, controller);
            }
        }
    } else {
        lv_obj_add_flag(applist, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(appload, LV_OBJ_FLAG_HIDDEN);
    }
}

