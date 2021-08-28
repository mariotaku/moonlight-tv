#include "priv.h"
#include "lvgl.h"
#include "util/logging.h"
#include "backend/application_manager.h"
#include "ui/manager.h"
#include "ui/streaming/overlay.h"

static void launcher_handle_server_updated(PPCMANAGER_RESP resp);

static void launcher_handle_apps_updated(PSERVER_LIST node);

static void _update_selected();

static PCMANAGER_CALLBACKS _pcmanager_callbacks = {
        .onAdded = launcher_handle_server_updated,
        .onUpdated = launcher_handle_server_updated,
        .prev = NULL,
        .next = NULL,
};
static APPMANAGER_CALLBACKS _appmanager_callbacks = {
        .onAppsUpdated = launcher_handle_apps_updated,
        .prev = NULL,
        .next = NULL,
};

void launcher_controller_init() {
    pcmanager_register_callbacks(&_pcmanager_callbacks);
    appmanager_register_callbacks(&_appmanager_callbacks);

    launcher_win_update_pclist();
    _update_selected();
}

void launcher_controller_destroy() {
    pcmanager_unregister_callbacks(&_pcmanager_callbacks);
    appmanager_unregister_callbacks(&_appmanager_callbacks);
}

void _update_selected(PSERVER_LIST node) {
    if (node && node->state.code == SERVER_STATE_ONLINE && !node->apps) {
        application_manager_load(node);
    }
    launcher_win_update_selected(node);
}

void launcher_handle_server_updated(PPCMANAGER_RESP resp) {
    launcher_win_update_pclist();
    _update_selected(launcher_win_selected_server());
}

void launcher_controller_pc_selected(lv_event_t *event) {
    _update_selected((PSERVER_LIST) event->user_data);
}

void launcher_open_game(lv_event_t *event) {
    PSERVER_LIST node = launcher_win_selected_server();
    STREAMING_SCENE_ARGS args = {
            .server = node->server,
            .app = (PAPP_DLIST) event->user_data
    };
    uimanager_push(lv_scr_act(), streaming_scene_create, &args);
}

void launcher_handle_apps_updated(PSERVER_LIST node) {
    PSERVER_LIST selected = launcher_win_selected_server();
    if (node != selected) return;
    _update_selected(selected);
}