#include <ui/settings/settings.controller.h>
#include "app.h"

#include "lvgl.h"

#include "priv.h"

static void open_settings(lv_event_t *event);

lv_obj_t *launcher_win_create(launcher_controller_t *controller, lv_obj_t *parent) {
    /*Create a window*/
    lv_obj_t *win = lv_win_create(parent, lv_dpx(40));

    lv_obj_t *header = lv_win_get_header(win);
    lv_obj_add_flag(header, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *content = lv_win_get_content(win);


    static lv_coord_t col_dsc[] = {LV_GRID_FR(2), LV_GRID_FR(5), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(content, col_dsc, row_dsc);

    lv_obj_t *nav = lv_obj_create(content);
    lv_obj_t *right = lv_obj_create(content);
    lv_obj_set_grid_cell(nav, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    lv_obj_set_grid_cell(right, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 0, 1);

    lv_obj_set_style_pad_row(nav, 0, 0);

    lv_obj_clear_flag(nav, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(nav, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(nav, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(nav, 0, 0);
    lv_obj_set_style_clip_corner(nav, true, 0);

    lv_obj_set_style_pad_all(right, 0, 0);

    lv_obj_t *pclist = lv_list_create(nav);
    lv_obj_set_width(pclist, LV_PCT(100));
    lv_obj_set_style_radius(pclist, 0, 0);
    lv_obj_set_style_border_width(pclist, 0, 0);
    lv_obj_set_style_outline_width(pclist, 0, 0);
    lv_obj_set_style_bg_opa(pclist, 0, 0);

    lv_obj_set_flex_grow(pclist, 1);

    // Use list button for normal container
    lv_obj_t *info_btn = lv_list_add_btn(nav, LV_SYMBOL_LIST, "Info");
    lv_obj_set_flex_grow(info_btn, 0);
    lv_obj_set_style_border_side(info_btn, LV_BORDER_SIDE_NONE, 0);

    lv_obj_t *add_btn = lv_list_add_btn(nav, LV_SYMBOL_PLUS, "Add");
    lv_obj_set_flex_grow(add_btn, 0);
    lv_obj_set_style_border_side(add_btn, LV_BORDER_SIDE_NONE, 0);
    lv_obj_t *pref_btn = lv_list_add_btn(nav, LV_SYMBOL_SETTINGS, "Settings");
    lv_obj_set_flex_grow(pref_btn, 0);
    lv_obj_set_style_border_side(pref_btn, LV_BORDER_SIDE_NONE, 0);
    lv_obj_t *close_btn = lv_list_add_btn(nav, LV_SYMBOL_CLOSE, "Exit");
    lv_obj_set_flex_grow(close_btn, 0);
    lv_obj_set_style_border_side(close_btn, LV_BORDER_SIDE_NONE, 0);

    lv_obj_add_event_cb(pref_btn, open_settings, LV_EVENT_CLICKED, controller);
    lv_obj_add_event_cb(close_btn, (lv_event_cb_t) app_request_exit, LV_EVENT_CLICKED, NULL);

    lv_obj_t *applist = lv_list_create(right);
    lv_obj_set_style_radius(applist, 0, 0);
    lv_obj_set_style_border_width(applist, 0, 0);
    lv_obj_set_style_outline_width(applist, 0, 0);
    lv_obj_set_style_bg_opa(applist, 0, 0);
    lv_obj_set_size(applist, LV_PCT(100), LV_PCT(100));
    lv_obj_t *appload = lv_spinner_create(right, 1000, 60);
    lv_obj_set_size(appload, lv_dpx(60), lv_dpx(60));
    lv_obj_center(appload);

    controller->applist = applist;
    controller->appload = appload;
    controller->pclist = pclist;
    return win;
}

static void open_settings(lv_event_t *event) {
    ui_view_controller_t *controller = event->user_data;
    uimanager_push(controller->manager, settings_controller, NULL);
}