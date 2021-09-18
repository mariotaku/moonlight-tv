#include "settings.controller.h"
#include "lvgl/ext/lv_obj_controller.h"

static void settings_close(lv_event_t *e);

lv_obj_t *settings_win_create(struct lv_obj_controller_t *self, lv_obj_t *parent) {
    settings_controller_t *controller = (settings_controller_t *) self;
    /*Create a window*/
    lv_obj_t *win = lv_win_create(parent, lv_dpx(50));
    lv_obj_t *title = lv_win_add_title(win, "Settings");
    lv_obj_set_style_text_font(title, lv_theme_get_font_large(win), 0);

    lv_obj_t *header = lv_win_get_header(win);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_ver(header, 15, 0);

    lv_obj_t *close_btn = lv_win_add_btn(win, LV_SYMBOL_CLOSE, lv_dpx(25));
    lv_obj_add_flag(close_btn, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_style_size(close_btn, lv_dpx(25), 0);
    lv_obj_set_style_radius(close_btn, LV_RADIUS_CIRCLE, 0);
    lv_group_remove_obj(close_btn);

    lv_obj_add_event_cb(close_btn, settings_close, LV_EVENT_CLICKED, controller);

    lv_obj_t *content = lv_win_get_content(win);
    static lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(2), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(content, col_dsc, row_dsc);
    lv_obj_t *nav = lv_list_create(content);
    lv_obj_set_child_group(nav, lv_group_create());
    controller->nav = nav;
    lv_obj_set_grid_cell(nav, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);

    lv_obj_t *detail = lv_obj_create(content);
    lv_obj_set_child_group(detail, lv_group_create());
    lv_obj_set_grid_cell(detail, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    controller->detail = detail;

    return win;
}


static void settings_close(lv_event_t *e) {
    lv_obj_controller_t *controller = e->user_data;
    lv_obj_controller_pop(controller);
}