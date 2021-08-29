#include "settings.controller.h"
#include "ui/manager.h"

static void settings_close(lv_event_t *e);

lv_obj_t *settings_win_create(struct ui_view_controller_t *controller, lv_obj_t *parent) {
    /*Create a window*/
    lv_obj_t *win = lv_win_create(parent, 80);
    lv_win_add_title(win, "Settings");

    lv_obj_t *header = lv_win_get_header(win);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_ver(header, 15, 0);

    lv_obj_t *close_btn = lv_win_add_btn(win, LV_SYMBOL_CLOSE, 50);
    lv_obj_set_style_size(close_btn, 50, 0);
    lv_obj_set_style_radius(close_btn, LV_RADIUS_CIRCLE, 0);

    lv_obj_add_event_cb(close_btn, settings_close, LV_EVENT_CLICKED, controller);

    lv_obj_t *content = lv_win_get_content(win);
    static lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(2), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(content, col_dsc, row_dsc);
    lv_obj_t *nav = lv_list_create(content);
    lv_obj_set_grid_cell(nav, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    lv_list_add_btn(nav, LV_SYMBOL_DUMMY, "Basic Settings");
    lv_list_add_btn(nav, LV_SYMBOL_DUMMY, "Host Settings");
    lv_list_add_btn(nav, LV_SYMBOL_KEYBOARD, "Input Settings");
    lv_list_add_btn(nav, LV_SYMBOL_VIDEO, "Decoder Settings");
    lv_list_add_btn(nav, LV_SYMBOL_DUMMY, "About");

    lv_obj_t *detail = lv_obj_create(content);
    lv_obj_set_grid_cell(detail, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 0, 1);

    return win;
}


static void settings_close(lv_event_t *e) {
    ui_view_controller_t *controller = e->user_data;
    uimanager_pop(controller->manager);
}