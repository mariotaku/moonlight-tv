//
// Created by Mariotaku on 2021/09/11.
//

#include "pref_obj.h"

lv_obj_t *pref_checkbox(lv_obj_t *parent, const char *title, bool *value, bool reverse) {
    lv_obj_t *checkbox = lv_checkbox_create(parent);
    lv_checkbox_set_text(checkbox, title);
    if (*value ^ reverse) {
        lv_obj_add_state(checkbox, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(checkbox, LV_STATE_CHECKED);
    }
    return checkbox;
}

lv_obj_t *pref_dropdown_int(lv_obj_t *parent, const pref_dropdown_int_entry_t *entries, int num_entries,
                            int *value) {
    lv_obj_t *dropdown = lv_dropdown_create(parent);
    lv_dropdown_clear_options(dropdown);
    int match_index = -1, fallback_index = -1;
    for (int i = 0; i < num_entries; ++i) {
        pref_dropdown_int_entry_t entry = entries[i];
        lv_dropdown_add_option(dropdown, entry.name, LV_DROPDOWN_POS_LAST);
        if (fallback_index < 0 && entry.fallback) {
            fallback_index = i;
        }
        if (*value == entry.value) {
            match_index = i;
        }
    }
    if (match_index >= 0) {
        lv_dropdown_set_selected(dropdown, match_index);
    } else if (fallback_index) {
        lv_dropdown_set_selected(dropdown, fallback_index);
    } else {
        lv_dropdown_set_selected(dropdown, 0);
    }
    return dropdown;
}

lv_obj_t *pref_dropdown_int_pair(lv_obj_t *parent, const pref_dropdown_int_pair_entry_t *entries, int num_entries,
                                 int *value_a, int *value_b) {
    lv_obj_t *dropdown = lv_dropdown_create(parent);
    lv_dropdown_clear_options(dropdown);
    int match_index = -1, fallback_index = -1;
    for (int i = 0; i < num_entries; ++i) {
        pref_dropdown_int_pair_entry_t entry = entries[i];
        lv_dropdown_add_option(dropdown, entry.name, LV_DROPDOWN_POS_LAST);
        if (fallback_index < 0 && entry.fallback) {
            fallback_index = i;
        }
        if (*value_a == entry.value_a && *value_b == entry.value_b) {
            match_index = i;
        }
    }
    if (match_index >= 0) {
        lv_dropdown_set_selected(dropdown, match_index);
    } else if (fallback_index) {
        lv_dropdown_set_selected(dropdown, fallback_index);
    } else {
        lv_dropdown_set_selected(dropdown, 0);
    }
    return dropdown;
}

lv_obj_t *pref_slider(lv_obj_t *parent, int *value, int min, int max, int step) {
    lv_obj_t *slider = lv_slider_create(parent);
    lv_slider_set_range(slider, min / step, max / step);
    lv_slider_set_value(slider, *value / step, LV_ANIM_OFF);
    return slider;
}