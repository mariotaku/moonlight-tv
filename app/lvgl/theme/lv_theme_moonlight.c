#include <app.h>
#include "lv_theme_moonlight.h"

static void apply_cb(struct _lv_theme_t *, lv_obj_t *);

static void lv_start_text_input(lv_event_t *event);

static void lv_dialog_destroy(lv_event_t *event);

static void dialog_group_focus_cb(lv_group_t *group);

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
    } else if (lv_obj_check_type(obj, &lv_dialog_class)) {
        lv_obj_set_width(obj, LV_SIZE_CONTENT);
        lv_obj_set_style_min_width(obj, LV_PCT(40), 0);
        lv_obj_set_style_max_width(obj, LV_PCT(60), 0);
        lv_obj_set_style_flex_cross_place(obj, LV_FLEX_ALIGN_END, 0);
        lv_group_t *group = lv_group_create();
        group->user_data = obj;
        lv_group_add_obj(group, obj);
        lv_group_set_focus_cb(group, dialog_group_focus_cb);
        lv_indev_set_group(app_indev_key, group);
        lv_obj_add_event_cb(obj, lv_dialog_destroy, LV_EVENT_DELETE, group);
    }
}

static void lv_start_text_input(lv_event_t *event) {
    lv_obj_t *target = lv_event_get_target(event);
    lv_area_t *coords = &target->coords;
    app_start_text_input(coords->x1, coords->y1, lv_area_get_width(coords), lv_area_get_height(coords));
}

static void lv_dialog_destroy(lv_event_t *event) {
    lv_indev_set_group(app_indev_key, lv_group_get_default());
    lv_group_t *group = lv_event_get_user_data(event);
    lv_group_remove_all_objs(group);
    lv_group_del(group);
}

static void dialog_group_focus_cb(lv_group_t *group) {
    if (lv_group_get_focused(group) == group->user_data) {
        lv_group_focus_next(group);
    }
}