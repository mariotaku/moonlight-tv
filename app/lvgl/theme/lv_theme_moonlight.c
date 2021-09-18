#include <app.h>
#include "lv_theme_moonlight.h"

static void apply_cb(struct _lv_theme_t *, lv_obj_t *);

static void lv_start_text_input(lv_event_t *event);

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
    if (lv_obj_has_class(obj, &lv_list_btn_class)) {
        lv_obj_set_style_bg_opa(obj, 0, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_FULL, LV_STATE_FOCUS_KEY);
//        lv_obj_set_style_border_width(obj, lv_dpx(2), LV_STATE_FOCUS_KEY);
        lv_obj_set_style_border_color(obj, theme->color_primary, LV_STATE_FOCUS_KEY);
    }
    if (lv_obj_has_class(obj, &lv_label_class)) {
        lv_obj_t *parent = lv_obj_get_parent(obj);
        if (parent) {
            lv_obj_t *parent2 = lv_obj_get_parent(parent);
            if (lv_obj_has_class(parent2, &lv_win_class) && lv_win_get_header(parent2) == parent) {
                lv_obj_set_style_text_font(obj, theme->font_large, 0);
            }
        }
    }
    if (lv_obj_check_type(obj, &lv_textarea_class)) {
        lv_obj_add_event_cb(obj, lv_start_text_input, LV_EVENT_FOCUSED, NULL);
        lv_obj_add_event_cb(obj, (lv_event_cb_t) app_stop_text_input, LV_EVENT_DEFOCUSED, NULL);
    } else if (lv_obj_check_type(obj, &lv_msgbox_class)) {
        lv_obj_set_width(obj, LV_PCT(40));
        lv_obj_set_style_min_width(obj, LV_PCT(40), 0);
        lv_obj_set_style_max_width(obj, LV_PCT(60), 0);
        lv_obj_set_style_flex_main_place(obj, LV_FLEX_ALIGN_END, 0);
        lv_group_t *group = lv_group_create();
        group->user_data = obj;
        lv_obj_set_child_group(obj, group);
        lv_indev_set_group(app_indev_key, group);
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

static void msgbox_destroy(lv_event_t *event) {
    lv_indev_set_group(app_indev_key, lv_group_get_default());
    lv_group_t *group = lv_event_get_user_data(event);
    lv_group_remove_all_objs(group);
    lv_group_del(group);
}