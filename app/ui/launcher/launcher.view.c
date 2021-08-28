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

lv_obj_t *launcher_win_create(lv_obj_t *parent, const void *args) {
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

    pclist = lv_list_create(nav);
    lv_obj_set_width(pclist, LV_PCT(100));
    lv_obj_set_style_radius(pclist, 0, 0);
    lv_obj_set_style_border_width(pclist, 0, 0);
    lv_obj_set_style_outline_width(pclist, 0, 0);
    lv_obj_set_style_bg_opa(pclist, 0, 0);

    lv_obj_set_flex_grow(pclist, 1);
//    lv_obj_set_flex_grow(menu, 0);

//    lv_obj_add_event_cb(pclist, pclist_dpad, LV_EVENT_KEY, NULL);
//    lv_obj_add_event_cb(pclist, pclist_predraw, LV_EVENT_DRAW_MAIN_BEGIN, NULL);

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

    lv_obj_add_event_cb(pref_btn, (lv_event_cb_t) ui_open_settings, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(close_btn, (lv_event_cb_t) app_request_exit, LV_EVENT_CLICKED, NULL);

    applist = lv_list_create(right);
    lv_obj_set_style_radius(applist, 0, 0);
    lv_obj_set_style_border_width(applist, 0, 0);
    lv_obj_set_style_outline_width(applist, 0, 0);
    lv_obj_set_style_bg_opa(applist, 0, 0);
    lv_obj_set_size(applist, LV_PCT(100), LV_PCT(100));
    appload = lv_spinner_create(right, 1000, 60);
    lv_obj_center(appload);

    launcher_controller_init();

    lv_obj_add_event_cb(win, (lv_event_cb_t) launcher_controller_destroy, LV_EVENT_DELETE, NULL);
    return win;
}

lv_indev_t *lv_indev_get_type_act(lv_indev_type_t type) {
    for (lv_indev_t *cur = lv_indev_get_act(); cur != NULL; cur = lv_indev_get_next(cur)) {
        if (cur->driver->type == type)
            return cur;
    }
    return NULL;
}

void launcher_win_update_pclist() {
    lv_obj_clean(pclist);
    for (PSERVER_LIST cur = computer_list; cur != NULL; cur = cur->next) {
        lv_obj_t *pcitem = lv_list_add_btn(pclist, NULL, cur->server->hostname);
        lv_obj_add_event_cb(pcitem, launcher_controller_pc_selected, LV_EVENT_CLICKED, cur);

    }
}

static PSERVER_LIST selected_server;

void launcher_win_update_selected(PSERVER_LIST node) {
    selected_server = node;
    lv_obj_clean(applist);
    if (!node) {
        lv_obj_add_flag(applist, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(appload, LV_OBJ_FLAG_HIDDEN);
    } else if (node->state.code == SERVER_STATE_ONLINE) {
        if (node->appload) {
            lv_obj_add_flag(applist, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(appload, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_clear_flag(applist, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(appload, LV_OBJ_FLAG_HIDDEN);
            for (PAPP_DLIST cur = node->apps; cur != NULL; cur = cur->next) {
                lv_obj_t *item = lv_list_add_btn(applist, NULL, cur->name);
                lv_obj_add_event_cb(item, launcher_open_game, LV_EVENT_CLICKED, cur);
            }
        }
    } else {
        lv_obj_add_flag(applist, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(appload, LV_OBJ_FLAG_HIDDEN);
    }
}

PSERVER_LIST launcher_win_selected_server() {
    return selected_server;
}