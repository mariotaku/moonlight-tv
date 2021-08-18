#include "priv.h"
#include "lvgl.h"
#include "util/logging.h"
#include "backend/application_manager.h"

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

void launcher_controller_init()
{
    pcmanager_register_callbacks(&_pcmanager_callbacks);
    appmanager_register_callbacks(&_appmanager_callbacks);

    launcher_win_update_pclist();
    _update_selected();
}

void launcher_controller_destroy()
{
    pcmanager_unregister_callbacks(&_pcmanager_callbacks);
    appmanager_unregister_callbacks(&_appmanager_callbacks);
}

void _update_selected()
{
    PSERVER_LIST node = launcher_win_selected_server();
    if (node->state.code == SERVER_STATE_ONLINE && !node->apps)
    {
        application_manager_load(node);
    }
    launcher_win_update_selected();
}

void launcher_handle_server_updated(PPCMANAGER_RESP resp)
{
    launcher_win_update_pclist();
    _update_selected();
}

void launcher_controller_pc_selected(lv_event_t *event)
{
    _update_selected();
}

void launcher_handle_apps_updated(PSERVER_LIST node)
{
    _update_selected();
}