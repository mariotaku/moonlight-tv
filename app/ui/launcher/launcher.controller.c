#include "priv.h"
#include "lvgl.h"
#include "util/logging.h"

static void launcher_handle_server_updated(PPCMANAGER_RESP resp);

static PCMANAGER_CALLBACKS _pcmanager_callbacks = {
    .onAdded = launcher_handle_server_updated,
    .onUpdated = launcher_handle_server_updated,
    .prev = NULL,
    .next = NULL,
};

void launcher_controller_init()
{
    pcmanager_register_callbacks(&_pcmanager_callbacks);
}

void launcher_controller_pc_selected(lv_event_t *event)
{
    int index = lv_dropdown_get_selected(event->target);
    applog_d("Launcher", "PC #%d selected", index);
}

void launcher_handle_server_updated(PPCMANAGER_RESP resp)
{
    launcher_win_update_pclist();
}