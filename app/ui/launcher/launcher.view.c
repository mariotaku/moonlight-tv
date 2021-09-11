#include <ui/settings/settings.controller.h>
#include "app.h"

#include "lvgl.h"

#include "launcher.controller.h"

static void open_manual_add(lv_event_t *event);

static void open_settings(lv_event_t *event);

lv_obj_t *launcher_win_create(lv_obj_controller_t *self, lv_obj_t *parent) {
    launcher_controller_t *controller = (launcher_controller_t *) self;
    /*Create a window*/
    lv_obj_t *win = lv_win_create(parent, lv_dpx(40));

    lv_obj_t *header = lv_win_get_header(win);
    lv_obj_add_flag(header, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *content = lv_win_get_content(win);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(content, 0, 0);

    lv_obj_t *nav = lv_obj_create(content);
    lv_obj_t *nav_shade = lv_obj_create(content);
    lv_obj_t *detail = lv_obj_create(content);
    lv_obj_set_size(nav, lv_dpx(200), LV_PCT(100));
    lv_obj_set_size(nav_shade, lv_dpx(200), LV_PCT(100));
    lv_obj_set_size(detail, lv_obj_get_width(parent) - lv_dpx(40), LV_PCT(100));
    lv_obj_align(detail, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_clear_flag(detail, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_pad_row(nav, 0, 0);

    lv_obj_clear_flag(nav, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(nav, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(nav, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(nav, 0, 0);
    lv_obj_set_style_radius(nav, 0, 0);
    lv_obj_set_style_border_width(nav, 0, 0);

    lv_obj_set_style_pad_all(nav_shade, 0, 0);
    lv_obj_set_style_radius(nav_shade, 0, 0);
    lv_obj_set_style_border_width(nav_shade, 0, 0);
    lv_obj_set_style_bg_color(nav_shade, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(nav_shade, LV_OPA_0, 0);
    lv_obj_set_style_bg_opa(nav_shade, LV_OPA_40, LV_STATE_USER_1);
    lv_obj_clear_flag(nav_shade, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(nav_shade, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_transition(nav_shade, &controller->tr_nav, 0);
    lv_obj_set_style_transition(nav_shade, &controller->tr_detail, LV_STATE_USER_1);

    lv_obj_set_style_pad_all(detail, 0, 0);
    lv_obj_set_style_radius(detail, 0, 0);
    lv_obj_set_style_bg_color(detail, lv_color_lighten(lv_color_black(), 30), 0);
    lv_obj_set_style_shadow_color(detail, lv_color_black(), 0);
    lv_obj_set_style_shadow_opa(detail, LV_OPA_MAX, 0);
    lv_obj_set_style_shadow_width(detail, lv_dpx(5), 0);
    lv_obj_set_style_border_width(detail, 0, 0);
    lv_obj_set_style_translate_x(detail, lv_dpx(200 - 40), 0);
    lv_obj_set_style_translate_x(detail, 0, LV_STATE_USER_1);
    lv_obj_set_style_transition(detail, &controller->tr_nav, 0);
    lv_obj_set_style_transition(detail, &controller->tr_detail, LV_STATE_USER_1);

    lv_obj_t *pclist = lv_list_create(nav);
    lv_obj_add_flag(pclist, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_width(pclist, LV_PCT(100));
    lv_obj_set_style_pad_all(pclist, 0, 0);
    lv_obj_set_style_radius(pclist, 0, 0);
    lv_obj_set_style_border_width(pclist, 0, 0);
    lv_obj_set_style_bg_opa(pclist, 0, 0);

    lv_obj_set_flex_grow(pclist, 1);

    // Use list button for normal container
    lv_obj_t *add_btn = lv_list_add_btn(nav, LV_SYMBOL_PLUS, "Add");
    lv_obj_add_flag(add_btn, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_flex_grow(add_btn, 0);
    lv_obj_set_style_border_side(add_btn, LV_BORDER_SIDE_NONE, 0);
    lv_obj_t *pref_btn = lv_list_add_btn(nav, LV_SYMBOL_SETTINGS, "Settings");
    lv_obj_add_flag(pref_btn, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_flex_grow(pref_btn, 0);
    lv_obj_set_style_border_side(pref_btn, LV_BORDER_SIDE_NONE, 0);
    lv_obj_t *exit_btn = lv_list_add_btn(nav, LV_SYMBOL_CLOSE, "Exit");
    lv_obj_add_flag(exit_btn, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_flex_grow(exit_btn, 0);
    lv_obj_set_style_border_side(exit_btn, LV_BORDER_SIDE_NONE, 0);

    lv_obj_add_event_cb(add_btn, open_manual_add, LV_EVENT_CLICKED, controller);
    lv_obj_add_event_cb(pref_btn, open_settings, LV_EVENT_CLICKED, controller);
    lv_obj_add_event_cb(exit_btn, app_request_exit, LV_EVENT_CLICKED, NULL);

    controller->nav = nav;
    controller->nav_shade = nav_shade;
    controller->detail = detail;
    controller->pclist = pclist;
    return win;
}

static void open_settings(lv_event_t *event) {
    lv_obj_controller_t *controller = event->user_data;
    lv_controller_manager_push(controller->manager, &settings_controller_cls, NULL);
}

static void manual_add_cb(const pcmanager_resp_t *resp, void *userdata);

static void manual_add_cb(const pcmanager_resp_t *resp, void *userdata) {

}

static void open_manual_add(lv_event_t *event) {
    lv_obj_controller_t *controller = event->user_data;
//    lv_obj_t *popup = lv_obj_create(controller->obj);
    pcmanager_manual_add(pcmanager, "192.168.4.16", manual_add_cb, controller);
}