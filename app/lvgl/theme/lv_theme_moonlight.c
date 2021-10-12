#include <app.h>
#include <lvgl/ext/lv_child_group.h>
#include "lv_theme_moonlight.h"

static void apply_cb(struct _lv_theme_t *, lv_obj_t *);

static void lv_start_text_input(lv_event_t *event);

static void msgbox_key(lv_event_t *event);

static void msgbox_destroy(lv_event_t *event);

void lv_theme_moonlight_init(lv_theme_t *theme) {
    lv_theme_set_apply_cb(theme, apply_cb);
    theme->font_small = &lv_font_montserrat_28;
    theme->font_large = &lv_font_montserrat_38;
}

static void apply_cb(lv_theme_t *theme, lv_obj_t *obj) {
    if (lv_obj_has_class(obj, &lv_btn_class)) {
        lv_obj_set_style_flex_cross_place(obj, LV_FLEX_ALIGN_CENTER, 0);
    }
    if (lv_obj_has_class(obj, &lv_label_class)) {
        lv_obj_t *parent = lv_obj_get_parent(obj);
        if (parent) {
            // Assume this is title
            if (lv_obj_check_type(parent, &lv_msgbox_class) && lv_msgbox_get_title(parent) == NULL) {
                lv_obj_set_style_text_font(obj, theme->font_large, 0);
            } else {
                lv_obj_t *parent2 = lv_obj_get_parent(parent);
                if (parent2 && lv_obj_has_class(parent2, &lv_win_class) && lv_win_get_header(parent2) == parent) {
                    lv_obj_set_style_text_font(obj, theme->font_large, 0);
                }
            }
        }
    }
    if (lv_obj_check_type(obj, &lv_textarea_class)) {
        lv_obj_add_event_cb(obj, lv_start_text_input, LV_EVENT_FOCUSED, NULL);
        lv_obj_add_event_cb(obj, (lv_event_cb_t) app_stop_text_input, LV_EVENT_DEFOCUSED, NULL);
    } else if (lv_obj_check_type(obj, &lv_msgbox_class)) {
        if (lv_obj_get_width(lv_scr_act()) / 10 * 4 > LV_DPI_DEF * 2) {
            lv_obj_set_width(obj, LV_PCT(40));
        } else {
            lv_obj_set_width(obj, LV_DPI_DEF * 2);
        }
        lv_obj_set_style_min_width(obj, LV_PCT(40), 0);
        lv_obj_set_style_max_width(obj, LV_PCT(60), 0);
        lv_obj_set_style_flex_main_place(obj, LV_FLEX_ALIGN_END, 0);
        lv_group_t *group = lv_group_create();
        group->user_data = app_input_get_modal_group();
        app_input_set_modal_group(group);
        lv_obj_add_event_cb(obj, cb_child_group_add, LV_EVENT_CHILD_CREATED, group);
        lv_obj_add_event_cb(obj, msgbox_key, LV_EVENT_KEY, NULL);
        lv_obj_add_event_cb(obj, msgbox_destroy, LV_EVENT_DELETE, group);
    } else if (lv_obj_check_type(obj, &lv_msgbox_backdrop_class)) {
        lv_obj_set_style_bg_color(obj, lv_color_black(), 0);
        lv_obj_set_style_bg_opa(obj, LV_OPA_50, 0);
    }
}

static void lv_start_text_input(lv_event_t *event) {
    lv_obj_t *target = lv_event_get_target(event);
    lv_area_t *coords = &target->coords;
    app_start_text_input(coords->x1, coords->y1, lv_area_get_width(coords), lv_area_get_height(coords));
}

static void msgbox_key(lv_event_t *event) {
    lv_obj_t *mbox = lv_event_get_current_target(event);
    lv_obj_t *target = lv_event_get_target(event);
    lv_group_t *group = lv_obj_get_group(target);
    if (!group) return;
    switch (lv_event_get_key(event)) {
        case LV_KEY_ESC: {
            lv_obj_t *btns = lv_msgbox_get_btns(mbox);
            if (btns && !lv_obj_has_flag(btns, LV_OBJ_FLAG_HIDDEN) &&
                !lv_btnmatrix_has_btn_ctrl(btns, 0, LV_BTNMATRIX_CTRL_DISABLED)) {
                lv_msgbox_close_async(mbox);
            }
            break;
        }
        case LV_KEY_UP: {
            lv_group_focus_prev(group);
            break;
        }
        case LV_KEY_DOWN: {
            lv_group_focus_next(group);
            break;
        }
        default: {
            break;
        }
    }
}

static void msgbox_destroy(lv_event_t *event) {
    lv_group_t *group = lv_event_get_user_data(event);
    if (app_input_get_modal_group() == group) {
        app_input_set_modal_group(group->user_data);
    }
    lv_group_remove_all_objs(group);
    lv_group_del(group);
}