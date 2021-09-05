#include <ui/settings/settings.controller.h>
#include "app.h"

#include "lvgl.h"

#include "launcher.controller.h"

static void open_settings(lv_event_t *event);

lv_obj_t *launcher_win_create(ui_view_controller_t *self, lv_obj_t *parent) {
    launcher_controller_t *controller = (launcher_controller_t *) self;
    /*Create a window*/
    lv_obj_t *win = lv_win_create(parent, lv_dpx(40));

    lv_obj_t *header = lv_win_get_header(win);
    lv_obj_add_flag(header, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *content = lv_win_get_content(win);
    lv_obj_set_style_pad_all(content, 0, 0);

    lv_obj_t *nav = lv_obj_create(content);
    lv_obj_t *right = lv_obj_create(content);
    lv_obj_set_size(nav, lv_dpx(200), LV_PCT(100));
    lv_obj_set_size(right, lv_obj_get_width(parent) - lv_dpx(40), LV_PCT(100));
    lv_obj_align(right, LV_ALIGN_RIGHT_MID, 0, 0);

    lv_obj_set_style_pad_row(nav, 0, 0);

    lv_obj_clear_flag(nav, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(nav, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(nav, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(nav, 0, 0);
    lv_obj_set_style_radius(nav, 0, 0);
//    lv_obj_set_style_bg_opa(nav, 0, 0);
    lv_obj_set_style_border_width(nav, 0, 0);

    lv_obj_set_style_pad_all(right, 0, 0);
    lv_obj_set_style_radius(right, 0, 0);
    lv_obj_set_style_bg_color(right, lv_color_lighten(lv_color_black(), 30), 0);
    lv_obj_set_style_shadow_color(right, lv_color_black(), 0);
    lv_obj_set_style_shadow_opa(right, LV_OPA_MAX, 0);
    lv_obj_set_style_shadow_width(right, lv_dpx(5), 0);
    lv_obj_set_style_border_width(right, 0, 0);

    lv_obj_t *pclist = lv_list_create(nav);
    lv_obj_set_width(pclist, LV_PCT(100));
    lv_obj_set_style_pad_all(pclist, 0, 0);
    lv_obj_set_style_radius(pclist, 0, 0);
    lv_obj_set_style_border_width(pclist, 0, 0);
    lv_obj_set_style_bg_opa(pclist, 0, 0);

    lv_obj_set_flex_grow(pclist, 1);

    // Use list button for normal container
    lv_obj_t *add_btn = lv_list_add_btn(nav, LV_SYMBOL_PLUS, "Add");
    lv_obj_set_flex_grow(add_btn, 0);
    lv_obj_set_style_border_side(add_btn, LV_BORDER_SIDE_NONE, 0);
    lv_obj_t *pref_btn = lv_list_add_btn(nav, LV_SYMBOL_SETTINGS, "Settings");
    lv_obj_set_flex_grow(pref_btn, 0);
    lv_obj_set_style_border_side(pref_btn, LV_BORDER_SIDE_NONE, 0);
    lv_obj_t *exit_btn = lv_list_add_btn(nav, LV_SYMBOL_CLOSE, "Exit");
    lv_obj_set_flex_grow(exit_btn, 0);
    lv_obj_set_style_border_side(exit_btn, LV_BORDER_SIDE_NONE, 0);

    lv_obj_add_event_cb(pref_btn, open_settings, LV_EVENT_CLICKED, controller);
    lv_obj_add_event_cb(exit_btn, app_request_exit, LV_EVENT_CLICKED, NULL);

    controller->right = right;
    controller->pclist = pclist;
    return win;
}

static void open_settings(lv_event_t *event) {
    ui_view_controller_t *controller = event->user_data;
    uimanager_push(controller->manager, settings_controller, NULL);
}