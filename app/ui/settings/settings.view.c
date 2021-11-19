#include "settings.controller.h"

#include "ui/root.h"

#include "lvgl/font/material_icons_regular_symbols.h"
#include "lvgl/util/lv_app_utils.h"

#include "util/font.h"
#include "util/i18n.h"

lv_obj_t *settings_win_create(struct lv_obj_controller_t *self, lv_obj_t *parent) {
    settings_controller_t *controller = (settings_controller_t *) self;
    /*Create a window*/
    lv_obj_t *win = lv_win_create(parent, LV_DPX(50));

    lv_obj_t *header = lv_win_get_header(win);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_ver(header, lv_dpx(10), 0);
    lv_obj_set_style_pad_gap(header, 0, 0);
    lv_obj_set_style_pad_left(header, 0, 0);
    lv_obj_set_style_pad_right(header, lv_dpx(20), 0);
    lv_obj_set_style_bg_color(header, lv_color_darken(lv_color_hex(0x2f3237), 4), 0);

    lv_obj_t *icon = lv_img_create(header);
    lv_obj_set_size(icon, LV_DPX(NAV_WIDTH_COLLAPSED), LV_DPX(50));
    lv_obj_set_style_pad_hor(icon, LV_DPX((NAV_WIDTH_COLLAPSED - NAV_LOGO_SIZE) / 2), 0);
    lv_obj_set_style_pad_ver(icon, LV_DPX((50 - NAV_LOGO_SIZE) / 2), 0);
    lv_img_set_src(icon, ui_logo_src());

    lv_obj_t *title = lv_win_add_title(win, locstr("Settings"));
    lv_obj_set_style_text_font(title, lv_theme_get_font_large(win), 0);

    lv_obj_t *close_btn = lv_win_add_btn(win, MAT_SYMBOL_CLOSE, lv_dpx(28));
    lv_btn_set_icon_font(close_btn, app_iconfonts.normal);
    lv_obj_add_flag(close_btn, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_style_size(close_btn, lv_dpx(28), 0);
    lv_obj_set_style_radius(close_btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(close_btn, lv_palette_darken(LV_PALETTE_GREY, 3), 0);
    lv_group_remove_obj(close_btn);
    controller->close_btn = close_btn;

    lv_obj_t *content = lv_win_get_content(win);
    lv_obj_set_style_pad_all(content, 0, 0);
    lv_obj_set_style_pad_gap(content, LV_DPX(2), 0);
    lv_obj_set_style_bg_color(content, lv_color_lighten(lv_color_black(), 30), 0);
    if (controller->pending_mini) {
        controller->nav = lv_obj_create(content);
        lv_obj_remove_style_all(controller->nav);
        lv_obj_center(controller->nav);
        controller->tabview = lv_tabview_create(content, LV_DIR_BOTTOM, LV_DPX(60));
    } else {
        static const lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(2), LV_GRID_TEMPLATE_LAST};
        static const lv_coord_t row_dsc[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
        lv_obj_set_grid_dsc_array(content, col_dsc, row_dsc);
        lv_obj_t *nav = lv_list_create(content);
        lv_obj_set_style_clip_corner(nav, false, 0);
        lv_obj_set_style_radius(nav, 0, 0);
        lv_obj_set_style_border_width(nav, 0, 0);
        controller->nav = nav;
        lv_obj_set_grid_cell(nav, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);

        lv_obj_t *detail = lv_obj_create(content);
        lv_obj_set_style_clip_corner(detail, false, 0);
        lv_obj_set_style_radius(detail, 0, 0);
        lv_obj_set_style_border_width(detail, 0, 0);
        lv_obj_set_grid_cell(detail, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
        controller->detail = detail;
    }
    controller->mini = controller->pending_mini;

    return win;
}