#include "settings.controller.h"
#include <stdlib.h>

ui_view_controller_t *settings_controller(const void *args) {
    ui_view_controller_t *controller = malloc(sizeof(ui_view_controller_t));
    lv_memset_00(controller, sizeof(ui_view_controller_t));
    controller->create_view = settings_win_create;
    controller->destroy_controller = free;
    return controller;
}