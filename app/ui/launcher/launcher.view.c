#include "app.h"

#include "lvgl.h"

#include "priv.h"
#include "backend/computer_manager.h"
#include "util/logging.h"

#include "ui/manager.h"

static lv_indev_t *lv_indev_get_type_act(lv_indev_type_t type);
static void pclist_predraw(lv_event_t *ev);
static void pclist_dpad(lv_event_t *ev);

lv_obj_t *pclist = NULL, *applist = NULL, *appload = NULL;

lv_obj_t *launcher_win_create()
{
    /*Create a window*/
    lv_obj_t *win = lv_win_create(lv_scr_act(), 60);

    lv_obj_t *header = lv_win_get_header(win);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_ver(header, 15, 0);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    pclist = lv_dropdown_create(header);
    lv_obj_add_event_cb(pclist, pclist_dpad, LV_EVENT_KEY, NULL);
    lv_obj_add_event_cb(pclist, pclist_predraw, LV_EVENT_DRAW_MAIN_BEGIN, NULL);
    lv_obj_set_style_min_width(pclist, 250, LV_STATE_DEFAULT);
    lv_obj_add_event_cb(pclist, launcher_controller_pc_selected, LV_EVENT_VALUE_CHANGED, NULL);

    lv_win_add_btn(win, LV_SYMBOL_BULLET, 30);

    lv_obj_t *spacing = lv_obj_create(header);
    lv_obj_set_style_bg_color(spacing, lv_obj_get_style_bg_color(header, LV_STATE_DEFAULT), LV_STATE_DEFAULT);

    lv_obj_set_flex_grow(spacing, 1);

    lv_obj_t *add_btn = lv_win_add_btn(win, LV_SYMBOL_PLUS, 30);

    lv_obj_t *pref_btn = lv_win_add_btn(win, LV_SYMBOL_SETTINGS, 30);
    lv_obj_add_event_cb(pref_btn, (lv_event_cb_t)ui_open_settings, LV_EVENT_CLICKED, NULL);

    lv_obj_t *close_btn = lv_win_add_btn(win, LV_SYMBOL_CLOSE, 30);

    lv_obj_add_event_cb(close_btn, (lv_event_cb_t)app_request_exit, LV_EVENT_CLICKED, NULL);

    lv_obj_t *content = lv_win_get_content(win);
    static lv_coord_t col_row_dsc[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(content, col_row_dsc, col_row_dsc);
    applist = lv_list_create(content);

    lv_obj_set_grid_cell(applist, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);

    appload = lv_spinner_create(content, 1000, 60);
    lv_obj_set_grid_cell(appload, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    launcher_controller_init();

    lv_obj_add_event_cb(win, (lv_event_cb_t)launcher_controller_destroy, LV_EVENT_DELETE, NULL);
    return win;
}

lv_indev_t *lv_indev_get_type_act(lv_indev_type_t type)
{
    for (lv_indev_t *cur = lv_indev_get_act(); cur != NULL; cur = lv_indev_get_next(cur))
    {
        if (cur->driver->type == type)
            return cur;
    }
    return NULL;
}

void launcher_win_update_pclist()
{
    lv_dropdown_clear_options(pclist);
    if (!computer_list)
    {
        lv_dropdown_add_option(pclist, "No Computer", 0);
    }
    else
    {
        for (PSERVER_LIST cur = computer_list; cur != NULL; cur = cur->next)
        {
            lv_dropdown_add_option(pclist, cur->server->hostname, LV_DROPDOWN_POS_LAST);
        }
    }
}

void launcher_win_update_selected()
{
    lv_obj_clean(applist);
    PSERVER_LIST node = launcher_win_selected_server();
    if (!node)
    {
        lv_obj_add_flag(applist, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(appload, LV_OBJ_FLAG_HIDDEN);
    }
    else if (node->state.code == SERVER_STATE_ONLINE)
    {
        if (node->appload)
        {
            lv_obj_add_flag(applist, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(appload, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_clear_flag(applist, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(appload, LV_OBJ_FLAG_HIDDEN);
            for (PAPP_DLIST cur = node->apps; cur != NULL; cur = cur->next)
            {
                lv_obj_t *item = lv_list_add_btn(applist, LV_SYMBOL_BULLET, cur->name);
            }
        }
    }
    else
    {
        lv_obj_add_flag(applist, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(appload, LV_OBJ_FLAG_HIDDEN);
    }
}

PSERVER_LIST launcher_win_selected_server()
{
    int index = lv_dropdown_get_selected(pclist);
    return serverlist_nth(computer_list, index);
}

static bool dropdown_opened = false;
void pclist_predraw(lv_event_t *e)
{
    dropdown_opened = lv_obj_get_state(pclist) & LV_STATE_CHECKED;
}

void pclist_dpad(lv_event_t *e)
{
    char c = *((char *)lv_event_get_param(e));
    if (!dropdown_opened)
    {
        if (c == LV_KEY_RIGHT || c == LV_KEY_LEFT)
        {
            lv_dropdown_close(pclist);
        }
    }
    applog_d("Launcher", "event %x", c);
}