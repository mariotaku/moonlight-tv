#include "window.h"
#include "ui/manager.h"

static void settings_win_close(lv_event_t *e);

lv_obj_t *settings_win_create()
{
    /*Create a window*/
    lv_obj_t *win = lv_win_create(lv_scr_act(), 60);
    lv_win_add_title(win, "Settings");

    lv_obj_t *header = lv_win_get_header(win);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_ver(header, 15, 0);

    lv_obj_t *close_btn = lv_win_add_btn(win, LV_SYMBOL_CLOSE, 30);
    lv_obj_add_event_cb(close_btn, settings_win_close, LV_EVENT_CLICKED, win);
    return win;
}

void settings_win_close(lv_event_t *e)
{
    uimanager_pop();
}