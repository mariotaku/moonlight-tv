#include "app.h"

#include "lv_theme_moonlight.h"

#include "util/font.h"
#include "lvgl/ext/lv_child_group.h"

static lv_style_t knob_shadow;

static void apply_cb(struct _lv_theme_t *, lv_obj_t *);

static void lv_start_text_input(lv_event_t *event);

static void msgbox_key(lv_event_t *event);

static void msgbox_cancel(lv_event_t *event);

static void msgbox_destroy(lv_event_t *event);

void lv_theme_moonlight_init(lv_theme_t *theme) {
    lv_theme_set_apply_cb(theme, apply_cb);

    lv_style_init(&knob_shadow);
    lv_style_set_shadow_color(&knob_shadow, lv_color_black());
    lv_style_set_shadow_width(&knob_shadow, LV_DPX(5));
    lv_style_set_shadow_opa(&knob_shadow, LV_OPA_50);
}

static void apply_cb(lv_theme_t *theme, lv_obj_t *obj) {
    bool set_font = true;
    if (lv_obj_has_class(obj, &lv_btn_class)) {
        lv_obj_set_style_flex_cross_place(obj, LV_FLEX_ALIGN_CENTER, 0);
    }
    if (lv_obj_has_class(obj, &lv_label_class)) {
        lv_obj_t *parent = lv_obj_get_parent(obj);
        if (parent) {
            // Assume this is title
            if (lv_obj_check_type(parent, &lv_msgbox_class) && lv_msgbox_get_title(parent) == NULL) {
                lv_obj_set_style_text_font(obj, theme->font_large, 0);
                set_font = false;
            } else {
                lv_obj_t *parent2 = lv_obj_get_parent(parent);
                if (parent2) {
                    if (lv_obj_has_class(parent2, &lv_win_class) && lv_win_get_header(parent2) == parent) {
                        lv_obj_set_style_text_font(obj, theme->font_large, 0);
                        set_font = false;
                    } else if (lv_obj_check_type(parent2, &lv_msgbox_class) &&
                               lv_msgbox_get_close_btn(parent2) == parent) {
                        lv_obj_set_style_text_font(obj, app_iconfonts.large, 0);
                        set_font = false;
                    }
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
        app_input_push_modal_group(group);
        lv_obj_add_event_cb(obj, cb_child_group_add, LV_EVENT_CHILD_CREATED, group);
        lv_obj_add_event_cb(obj, msgbox_key, LV_EVENT_KEY, NULL);
        lv_obj_add_event_cb(obj, msgbox_cancel, LV_EVENT_CANCEL, NULL);
        lv_obj_add_event_cb(obj, msgbox_destroy, LV_EVENT_DELETE, group);
    } else if (lv_obj_check_type(obj, &lv_msgbox_backdrop_class)) {
        lv_obj_set_style_bg_color(obj, lv_color_black(), 0);
        lv_obj_set_style_bg_opa(obj, LV_OPA_50, 0);
    } else if (lv_obj_check_type(obj, &lv_btnmatrix_class)) {
        lv_obj_t *parent = lv_obj_get_parent(obj);
        if (lv_obj_check_type(parent, &lv_msgbox_class)) {
            lv_obj_set_style_text_font(obj, theme->font_small, 0);
            set_font = false;
        }
    } else if (lv_obj_check_type(obj, &lv_dropdown_class)) {
        lv_obj_set_style_text_font(obj, app_iconfonts.large, LV_PART_INDICATOR);
        lv_dropdown_set_symbol(obj, MAT_SYMBOL_ARROW_DROP_DOWN);
    } else if (lv_obj_check_type(obj, &lv_checkbox_class)) {
        lv_obj_set_style_text_font(obj, app_iconfonts.large, LV_PART_INDICATOR | LV_STATE_CHECKED);
    } else if (lv_obj_check_type(obj, &lv_slider_class)) {
        lv_obj_add_style(obj, &knob_shadow, LV_PART_KNOB);
    }
    if (set_font) {
        lv_obj_set_style_text_font(obj, theme->font_normal, 0);
    }
}

static void lv_start_text_input(lv_event_t *event) {
    lv_obj_t *target = lv_event_get_target(event);
    lv_area_t *coords = &target->coords;
    app_start_text_input(coords->x1, coords->y1, lv_area_get_width(coords), lv_area_get_height(coords));
}

static void msgbox_key(lv_event_t *event) {
    lv_obj_t *target = lv_event_get_target(event);
    lv_group_t *group = lv_obj_get_group(target);
    if (!group) return;
    switch (lv_event_get_key(event)) {
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

static void msgbox_cancel(lv_event_t *event) {
    lv_obj_t *mbox = lv_event_get_current_target(event);
    lv_obj_t *target = lv_event_get_target(event);
    lv_group_t *group = lv_obj_get_group(target);
    if (!group) return;
    lv_obj_t *btns = lv_msgbox_get_btns(mbox);
    if (btns && !lv_obj_has_flag(btns, LV_OBJ_FLAG_HIDDEN) &&
        !lv_btnmatrix_has_btn_ctrl(btns, 0, LV_BTNMATRIX_CTRL_DISABLED)) {
        lv_msgbox_close(mbox);
    }
}

static void msgbox_destroy(lv_event_t *event) {
    lv_group_t *group = lv_event_get_user_data(event);
    app_input_remove_modal_group(group);
    lv_group_remove_all_objs(group);
    lv_group_del(group);
}