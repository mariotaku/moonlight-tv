#include "manager.h"

#include "lvgl.h"

void ui_init()
{
    /*Create a window*/
    lv_obj_t *win = lv_win_create(lv_scr_act(), 40);

    lv_obj_t *header = lv_win_get_header(win);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t *dd = lv_dropdown_create(header);
    lv_dropdown_set_options(dd, "Apple\n"
                                "Banana\n"
                                "Orange\n"
                                "Cherry\n"
                                "Grape\n"
                                "Raspberry\n"
                                "Melon\n"
                                "Orange\n"
                                "Lemon\n"
                                "Nuts");

    lv_obj_t *spacing = lv_obj_create(header);
    lv_obj_set_style_bg_color(spacing, lv_obj_get_style_bg_color(header, LV_STATE_DEFAULT), LV_STATE_DEFAULT);

    lv_obj_set_flex_grow(spacing, 1);

    // lv_obj_add_event_cb(dd, event_handler, LV_EVENT_ALL, NULL);

    lv_win_add_btn(win, LV_SYMBOL_PLUS, 30);

    lv_win_add_btn(win, LV_SYMBOL_SETTINGS, 30);

    lv_obj_t *close_btn = lv_win_add_btn(win, LV_SYMBOL_CLOSE, 30);
    lv_obj_align(close_btn, LV_ALIGN_RIGHT_MID, 0, 0);
    // lv_obj_add_event_cb(close_btn, lv_win_close_event_cb);

    /*Add some dummy content*/
    lv_obj_t *txt = lv_label_create(win);
    lv_label_set_text(txt, "This is the content of the window\n\n"
                           "You can add control buttons to\n"
                           "the window header\n\n"
                           "The content area becomes\n"
                           "automatically scrollable is it's \n"
                           "large enough.\n\n"
                           " You can scroll the content\n"
                           "See the scroll bar on the right!");
}