#include "lvgl.h"

#include "priv.h"
#include "backend/computer_manager.h"

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
    lv_obj_set_style_min_width(pclist, 250, LV_STATE_DEFAULT);
    lv_obj_add_event_cb(pclist, launcher_controller_pc_selected, LV_EVENT_VALUE_CHANGED, NULL);

    lv_win_add_btn(win, LV_SYMBOL_BULLET, 30);

    lv_obj_t *spacing = lv_obj_create(header);
    lv_obj_set_style_bg_color(spacing, lv_obj_get_style_bg_color(header, LV_STATE_DEFAULT), LV_STATE_DEFAULT);

    lv_obj_set_flex_grow(spacing, 1);

    lv_win_add_btn(win, LV_SYMBOL_PLUS, 30);

    lv_win_add_btn(win, LV_SYMBOL_SETTINGS, 30);

    lv_obj_t *close_btn = lv_win_add_btn(win, LV_SYMBOL_CLOSE, 30);
    // lv_obj_add_event_cb(close_btn, lv_win_close_event_cb);

    lv_obj_t *content = lv_win_get_content(win);
    static lv_coord_t col_row_dsc[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(content, col_row_dsc, col_row_dsc);
    applist = lv_list_create(content);
    lv_obj_set_grid_cell(applist, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);

    appload = lv_spinner_create(content, 1000, 60);
    lv_obj_set_grid_cell(appload, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);

    launcher_controller_init();
    return win;
}

void launcher_win_update_pclist()
{
    lv_dropdown_clear_options(pclist);
    for (PSERVER_LIST cur = computer_list; cur != NULL; cur = cur->next)
    {
        lv_dropdown_add_option(pclist, cur->server->hostname, LV_DROPDOWN_POS_LAST);
    }
}

void launcher_win_update_selected()
{
    static lv_group_t *group = NULL;
    if (group)
    {
        lv_group_remove_all_objs(group);
        lv_group_del(group);
        group = NULL;
    }
    lv_obj_clean(applist);
    PSERVER_LIST node = launcher_win_selected_server();
    if (node->state.code == SERVER_STATE_ONLINE)
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
            group = lv_group_create();
            for (PAPP_DLIST cur = node->apps; cur != NULL; cur = cur->next)
            {
                lv_obj_t *item = lv_list_add_btn(applist, LV_SYMBOL_BULLET, cur->name);
                lv_group_add_obj(group, item);
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