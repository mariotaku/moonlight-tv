#include "settings.controller.h"
#include <stdlib.h>
#include "panes/basic.pane.h"

typedef struct {
    const char *icon;
    const char *name;
    const lv_obj_controller_class_t *cls;
} settings_entry_t;

static const settings_entry_t entries[] = {
        {LV_SYMBOL_DUMMY,    "Basic Settings",   &settings_pane_basic_cls},
        {LV_SYMBOL_DUMMY,    "Host Settings",    &settings_pane_basic_cls},
        {LV_SYMBOL_KEYBOARD, "Input Settings",   &settings_pane_basic_cls},
        {LV_SYMBOL_VIDEO,    "Decoder Settings", &settings_pane_basic_cls},
        {LV_SYMBOL_DUMMY,    "About",            &settings_pane_basic_cls},
};
static const int entries_len = sizeof(entries) / sizeof(settings_entry_t);

static void on_view_created(lv_obj_controller_t *controller, lv_obj_t *view);

static void on_destroy_view(lv_obj_controller_t *controller, lv_obj_t *view);

static void on_entry_click(lv_event_t *event);

static void settings_controller_ctor(lv_obj_controller_t *self, void *args);

const lv_obj_controller_class_t settings_controller_cls = {
        .constructor_cb = settings_controller_ctor,
        .create_obj_cb = settings_win_create,
        .obj_created_cb = on_view_created,
        .obj_deleted_cb = on_destroy_view,
        .instance_size = sizeof(settings_controller_t),
};

static void settings_controller_ctor(lv_obj_controller_t *self, void *args) {

}

static void on_view_created(lv_obj_controller_t *self, lv_obj_t *view) {
    settings_controller_t *controller = (settings_controller_t *) self;
    controller->pane_manager = lv_controller_manager_create(controller->detail);

    for (int i = 0; i < entries_len; ++i) {
        settings_entry_t entry = entries[i];
        lv_obj_t *item_view = lv_list_add_btn(controller->nav, entry.icon, entry.name);
        item_view->user_data = (void *) entry.cls;
        lv_obj_add_event_cb(item_view, on_entry_click, LV_EVENT_CLICKED, controller);
    }
}

static void on_destroy_view(lv_obj_controller_t *self, lv_obj_t *view) {
    settings_controller_t *controller = (settings_controller_t *) self;
    lv_controller_manager_del(controller->pane_manager);
}

static void on_entry_click(lv_event_t *event) {
    settings_controller_t *controller = event->user_data;
    lv_controller_manager_replace(controller->pane_manager, event->target->user_data, NULL);
}