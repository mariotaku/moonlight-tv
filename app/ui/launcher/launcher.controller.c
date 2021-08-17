#include "priv.h"
#include "lvgl.h"
#include "util/logging.h"
#include "backend/application_manager.h"

static void launcher_handle_server_updated(PPCMANAGER_RESP resp);
static void launcher_handle_apps_updated(PSERVER_LIST node);

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
}

static void _update_selected()
{
    launcher_win_update_pclist();

    PSERVER_LIST node = launcher_win_selected_server();
    if (node->state.code == SERVER_STATE_ONLINE && !node->apps)
    {
        application_manager_load(node);
    }
}

void launcher_controller_pc_selected(lv_event_t *event)
{
    _update_selected();
}

void launcher_handle_server_updated(PPCMANAGER_RESP resp)
{
    _update_selected();
    launcher_win_update_selected();
}

void launcher_handle_apps_updated(PSERVER_LIST node)
{
    launcher_win_update_selected();
}