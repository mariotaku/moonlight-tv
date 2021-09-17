#include "ui/settings/settings.controller.h"
#include "lvgl/util/lv_app_utils.h"
#include "lvgl/font/symbols_material_icon.h"
#include "app.h"

#include "lvgl.h"

#include "launcher.controller.h"

static void open_settings(lv_event_t *event);

static void setup_shade(const launcher_controller_t *controller, lv_obj_t *shade, bool invert);

#define NAV_WIDTH_EXPANDED 240
#define NAV_WIDTH_COLLAPSED 44
#define NAV_TRANSLATE_OFFSET (NAV_WIDTH_EXPANDED - NAV_WIDTH_COLLAPSED)

lv_obj_t *launcher_win_create(lv_obj_controller_t *self, lv_obj_t *parent) {
    launcher_controller_t *controller = (launcher_controller_t *) self;
    /*Create a window*/
    lv_obj_t *win = lv_win_create(parent, lv_dpx(NAV_WIDTH_COLLAPSED));

    lv_obj_t *header = lv_win_get_header(win);
    lv_obj_add_flag(header, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *content = lv_win_get_content(win);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(content, LV_LAYOUT_GRID);
    controller->col_dsc[0] = lv_dpx(NAV_WIDTH_COLLAPSED);
    controller->col_dsc[1] = lv_dpx(NAV_TRANSLATE_OFFSET);
    controller->col_dsc[2] = LV_GRID_FR(1);
    controller->col_dsc[3] = LV_GRID_TEMPLATE_LAST;
    controller->row_dsc[0] = LV_GRID_FR(1);
    controller->row_dsc[1] = LV_GRID_TEMPLATE_LAST;
    lv_obj_set_style_pad_all(content, 0, 0);
    lv_obj_set_style_pad_gap(content, 0, 0);
    lv_obj_set_grid_dsc_array(content, controller->col_dsc, controller->row_dsc);

    lv_obj_t *nav = lv_obj_create(content);
    lv_obj_t *detail = lv_obj_create(content);

    lv_obj_set_grid_cell(nav, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_STRETCH, 0, 1);
    lv_obj_set_grid_cell(detail, LV_GRID_ALIGN_STRETCH, 1, 2, LV_GRID_ALIGN_STRETCH, 0, 1);
    lv_obj_clear_flag(detail, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_pad_row(nav, 0, 0);

    lv_obj_clear_flag(nav, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(nav, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(nav, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(nav, 0, 0);
    lv_obj_set_style_radius(nav, 0, 0);
    lv_obj_set_style_border_width(nav, 0, 0);

    lv_obj_set_style_pad_all(detail, 0, 0);
    lv_obj_set_style_radius(detail, 0, 0);
    lv_obj_set_style_bg_color(detail, lv_color_lighten(lv_color_black(), 30), 0);
    lv_obj_set_style_shadow_color(detail, lv_color_black(), 0);
    lv_obj_set_style_shadow_opa(detail, LV_OPA_MAX, 0);
    lv_obj_set_style_shadow_width(detail, lv_dpx(5), 0);
    lv_obj_set_style_border_width(detail, 0, 0);
    lv_obj_set_style_translate_x(detail, lv_dpx(NAV_TRANSLATE_OFFSET), 0);
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
    lv_obj_t *add_btn = lv_list_add_btn(nav, MAT_SYMBOL_ADD_TO_QUEUE, "Add");
    lv_obj_set_icon_font(add_btn, LV_ICON_FONT_DEFAULT);
    lv_obj_add_flag(add_btn, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_flex_grow(add_btn, 0);
    lv_obj_set_style_border_side(add_btn, LV_BORDER_SIDE_NONE, 0);
    lv_obj_t *pref_btn = lv_list_add_btn(nav, MAT_SYMBOL_SETTINGS, "Settings");
    lv_obj_set_icon_font(pref_btn, LV_ICON_FONT_DEFAULT);
    lv_obj_add_flag(pref_btn, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_flex_grow(pref_btn, 0);
    lv_obj_set_style_border_side(pref_btn, LV_BORDER_SIDE_NONE, 0);
    lv_obj_t *exit_btn = lv_list_add_btn(nav, MAT_SYMBOL_CLOSE, "Exit");
    lv_obj_set_icon_font(exit_btn, LV_ICON_FONT_DEFAULT);
    lv_obj_add_flag(exit_btn, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_flex_grow(exit_btn, 0);
    lv_obj_set_style_border_side(exit_btn, LV_BORDER_SIDE_NONE, 0);

    lv_obj_add_event_cb(pref_btn, open_settings, LV_EVENT_CLICKED, controller);
    lv_obj_add_event_cb(exit_btn, app_request_exit, LV_EVENT_CLICKED, NULL);

    controller->nav = nav;
    controller->detail = detail;
    controller->pclist = pclist;
    controller->add_btn = add_btn;
    return win;
}

static void setup_shade(const launcher_controller_t *controller, lv_obj_t *shade, bool invert) {
    lv_obj_set_style_pad_all(shade, 0, 0);
    lv_obj_set_style_radius(shade, 0, 0);
    lv_obj_set_style_border_width(shade, 0, 0);
    lv_obj_set_style_bg_color(shade, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(shade, LV_OPA_0, invert ? LV_STATE_USER_1 : 0);
    lv_obj_set_style_bg_opa(shade, LV_OPA_40, invert ? 0 : LV_STATE_USER_1);
    lv_obj_clear_flag(shade, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(shade, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_transition(shade, &controller->tr_nav, invert ? LV_STATE_USER_1 : 0);
    lv_obj_set_style_transition(shade, &controller->tr_detail, invert ? 0 : LV_STATE_USER_1);
}

static void open_settings(lv_event_t *event) {
    lv_obj_controller_t *controller = event->user_data;
    lv_controller_manager_push(controller->manager, &settings_controller_cls, NULL);
}