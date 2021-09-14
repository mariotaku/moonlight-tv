//
// Created by Mariotaku on 2021/09/11.
//

#include "pref_obj.h"

typedef union pref_attrs_t {
    struct {
        bool *ref;
        bool reverse;
    } checkbox;
    struct {
        int *ref;
        int step;
    } slider;
    struct {
        int *ref;
        const pref_dropdown_int_entry_t *entries;
    } dropdown_int;
    struct {
        int *ref_a, *ref_b;
        const pref_dropdown_int_pair_entry_t *entries;
    } dropdown_int_pair;
    struct {
        char **ref;
        const pref_dropdown_string_entry_t *entries;
    } dropdown_string;
} pref_attrs_t;

static void pref_attrs_free(lv_event_t *event);

static void pref_checkbox_value_write_back(lv_event_t *event);

static void pref_dropdown_int_change_cb(lv_event_t *event);

static void pref_dropdown_int_pair_change_cb(lv_event_t *event);

static void pref_dropdown_string_change_cb(lv_event_t *event);

static void pref_slider_value_write_back(lv_event_t *event);

lv_obj_t *pref_checkbox(lv_obj_t *parent, const char *title, bool *value, bool reverse) {
    lv_obj_t *checkbox = lv_checkbox_create(parent);
    lv_checkbox_set_text(checkbox, title);
    pref_attrs_t *attrs = lv_mem_alloc(sizeof(pref_attrs_t));
    attrs->checkbox.ref = value;
    attrs->checkbox.reverse = reverse;
    lv_obj_add_event_cb(checkbox, pref_checkbox_value_write_back, LV_EVENT_CLICKED, attrs);
    lv_obj_add_event_cb(checkbox, pref_attrs_free, LV_EVENT_DELETE, attrs);
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

    pref_attrs_t *attrs = lv_mem_alloc(sizeof(pref_attrs_t));
    attrs->dropdown_int.ref = value;
    attrs->dropdown_int.entries = entries;
    lv_obj_add_event_cb(dropdown, pref_dropdown_int_change_cb, LV_EVENT_VALUE_CHANGED, attrs);
    lv_obj_add_event_cb(dropdown, pref_attrs_free, LV_EVENT_DELETE, attrs);
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

    pref_attrs_t *attrs = lv_mem_alloc(sizeof(pref_attrs_t));
    attrs->dropdown_int_pair.ref_a = value_a;
    attrs->dropdown_int_pair.ref_b = value_b;
    attrs->dropdown_int_pair.entries = entries;
    lv_obj_add_event_cb(dropdown, pref_dropdown_int_pair_change_cb, LV_EVENT_VALUE_CHANGED, attrs);
    lv_obj_add_event_cb(dropdown, pref_attrs_free, LV_EVENT_DELETE, attrs);
    return dropdown;
}

lv_obj_t *pref_dropdown_string(lv_obj_t *parent, const pref_dropdown_string_entry_t *entries, int num_entries,
                               char **value) {
    lv_obj_t *dropdown = lv_dropdown_create(parent);
    lv_dropdown_clear_options(dropdown);
    int match_index = -1, fallback_index = -1;
    for (int i = 0; i < num_entries; ++i) {
        pref_dropdown_string_entry_t entry = entries[i];
        lv_dropdown_add_option(dropdown, entry.name, LV_DROPDOWN_POS_LAST);
        if (fallback_index < 0 && entry.fallback) {
            fallback_index = i;
        }
        if (strcmp(*value, entry.value) == 0) {
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

    pref_attrs_t *attrs = lv_mem_alloc(sizeof(pref_attrs_t));
    attrs->dropdown_string.ref = value;
    attrs->dropdown_string.entries = entries;
    lv_obj_add_event_cb(dropdown, pref_dropdown_string_change_cb, LV_EVENT_VALUE_CHANGED, attrs);
    lv_obj_add_event_cb(dropdown, pref_attrs_free, LV_EVENT_DELETE, attrs);
    return dropdown;
}

lv_obj_t *pref_slider(lv_obj_t *parent, int *value, int min, int max, int step) {
    lv_obj_t *slider = lv_slider_create(parent);
    lv_slider_set_range(slider, min / step, max / step);
    lv_slider_set_value(slider, *value / step, LV_ANIM_OFF);

    pref_attrs_t *attrs = lv_mem_alloc(sizeof(pref_attrs_t));
    attrs->slider.ref = value;
    attrs->slider.step = step;
    lv_obj_add_event_cb(slider, pref_slider_value_write_back, LV_EVENT_VALUE_CHANGED, attrs);
    lv_obj_add_event_cb(slider, pref_attrs_free, LV_EVENT_DELETE, attrs);
    return slider;
}

lv_obj_t *pref_title_label(lv_obj_t *parent, const char *title) {
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, title);
    lv_obj_set_size(label, LV_PCT(100), LV_SIZE_CONTENT);
    return label;
}

static void pref_attrs_free(lv_event_t *event) {
    lv_mem_free(lv_event_get_user_data(event));
}

static void pref_checkbox_value_write_back(lv_event_t *event) {
    pref_attrs_t *attrs = lv_event_get_user_data(event);
    bool checked = lv_obj_has_state(lv_event_get_current_target(event), LV_STATE_CHECKED);
    *attrs->checkbox.ref = checked ^ attrs->checkbox.reverse;
}

static void pref_dropdown_int_change_cb(lv_event_t *event) {
    pref_attrs_t *attrs = lv_event_get_user_data(event);
    int index = lv_dropdown_get_selected(lv_event_get_current_target(event));
    *attrs->dropdown_int.ref = attrs->dropdown_int.entries[index].value;
}

static void pref_dropdown_int_pair_change_cb(lv_event_t *event) {
    pref_attrs_t *attrs = lv_event_get_user_data(event);
    int index = lv_dropdown_get_selected(lv_event_get_current_target(event));
    *attrs->dropdown_int_pair.ref_a = attrs->dropdown_int_pair.entries[index].value_a;
    *attrs->dropdown_int_pair.ref_b = attrs->dropdown_int_pair.entries[index].value_b;
}

static void pref_dropdown_string_change_cb(lv_event_t *event) {
    pref_attrs_t *attrs = lv_event_get_user_data(event);
    int index = lv_dropdown_get_selected(lv_event_get_current_target(event));
    pref_dropdown_string_entry_t entry = attrs->dropdown_string.entries[index];
    char *new_value = entry.value ? strdup(entry.value) : NULL;
    *attrs->dropdown_string.ref = new_value;
}

static void pref_slider_value_write_back(lv_event_t *event) {
    pref_attrs_t *attrs = lv_event_get_user_data(event);
    int value = lv_slider_get_value(lv_event_get_current_target(event));
    *attrs->slider.ref = value * attrs->slider.step;
}