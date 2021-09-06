#include "settings.controller.h"
#include <stdlib.h>
#include "panes/basic.pane.h"

typedef struct {
    const char *icon;
    const char *name;
    lv_obj_controller_ctor_t ctor;
} settings_entry_t;

static const settings_entry_t entries[] = {
        {LV_SYMBOL_DUMMY,    "Basic Settings",   settings_pane_basic},
        {LV_SYMBOL_DUMMY,    "Host Settings",    settings_pane_basic},
        {LV_SYMBOL_KEYBOARD, "Input Settings",   settings_pane_basic},
        {LV_SYMBOL_VIDEO,    "Decoder Settings", settings_pane_basic},
        {LV_SYMBOL_DUMMY,    "About",            settings_pane_basic},
};
static const int entries_len = sizeof(entries) / sizeof(settings_entry_t);

static void on_view_created(lv_obj_controller_t *controller, lv_obj_t *view);

static void on_destroy_view(lv_obj_controller_t *controller, lv_obj_t *view);

static void on_entry_click(lv_event_t *event);

lv_obj_controller_t *settings_controller(void *args) {
    settings_controller_t *controller = lv_mem_alloc(sizeof(settings_controller_t));
    lv_memset_00(controller, sizeof(settings_controller_t));
    controller->base.create_view = settings_win_create;
    controller->base.view_created = on_view_created;
    controller->base.destroy_view = on_destroy_view;
    controller->base.destroy_controller = ui_view_controller_free;
    return (lv_obj_controller_t *) controller;
}

static void on_view_created(lv_obj_controller_t *self, lv_obj_t *view) {
    settings_controller_t *controller = (settings_controller_t *) self;
    controller->pane_manager = uimanager_new(controller->detail);

    for (int i = 0; i < entries_len; ++i) {
        settings_entry_t entry = entries[i];
        lv_obj_t *item_view = lv_list_add_btn(controller->nav, entry.icon, entry.name);
        item_view->user_data = entry.ctor;
        lv_obj_add_event_cb(item_view, on_entry_click, LV_EVENT_CLICKED, controller);
    }
}

static void on_destroy_view(lv_obj_controller_t *self, lv_obj_t *view) {
    settings_controller_t *controller = (settings_controller_t *) self;
    uimanager_destroy(controller->pane_manager);
}

static void on_entry_click(lv_event_t *event) {
    settings_controller_t *controller = event->user_data;
    uimanager_replace(controller->pane_manager, event->target->user_data, NULL);
}