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

        bool (*write_predicate)(int);
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

static const lv_obj_class_t pref_label_cls = {
    .base_class = &lv_label_class,
    .group_def = LV_OBJ_CLASS_GROUP_DEF_TRUE,
    .instance_size = sizeof(lv_label_t),
};

static void pref_attrs_free(lv_event_t *event);

static void pref_checkable_value_write_back(lv_event_t *event);

static void pref_checkable_dpad_check_restore(lv_event_t *event);

static void pref_dropdown_int_change_cb(lv_event_t *event);

static void pref_dropdown_int_pair_change_cb(lv_event_t *event);

static void pref_dropdown_string_change_cb(lv_event_t *event);

static void pref_dropdown_key_hack_cb(lv_event_t *event);

static void pref_dropdown_key_hack_del_cb(lv_event_t *event);

static void pref_slider_value_write_back(lv_event_t *event);

static void pref_dropdown_key_hack(lv_obj_t *dropdown);

typedef struct pref_dropdown_key_hack_state_t {
    bool opened;
} pref_dropdown_key_hack_state_t;

lv_obj_t *pref_pane_container(lv_obj_t *parent) {
    lv_obj_t *view = lv_obj_create(parent);
    lv_obj_add_flag(view, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_clear_flag(view, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(view, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(view, LV_OPA_TRANSP, 0);
    lv_obj_set_style_radius(view, 0, 0);
    lv_obj_set_style_border_opa(view, LV_OPA_TRANSP, 0);
    return view;
}

lv_obj_t *pref_checkbox(lv_obj_t *parent, const char *title, bool *value, bool reverse) {
    lv_obj_t *checkbox = lv_checkbox_create(parent);
    lv_checkbox_set_text(checkbox, title);
    lv_obj_set_size(checkbox, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(checkbox, LV_DPX(10), 0);
    lv_obj_set_style_radius(checkbox, LV_DPX(4), 0);
    pref_attrs_t *attrs = lv_mem_alloc(sizeof(pref_attrs_t));
    attrs->checkbox.ref = value;
    attrs->checkbox.reverse = reverse;
    lv_obj_add_event_cb(checkbox, pref_checkable_value_write_back, LV_EVENT_CLICKED, attrs);
    lv_obj_add_event_cb(checkbox, pref_checkable_dpad_check_restore, LV_EVENT_KEY, attrs);
    lv_obj_add_event_cb(checkbox, pref_attrs_free, LV_EVENT_DELETE, attrs);
    if (*value ^ reverse) {
        lv_obj_add_state(checkbox, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(checkbox, LV_STATE_CHECKED);
    }
    return checkbox;
}

lv_obj_t *pref_dropdown_int(lv_obj_t *parent, const pref_dropdown_int_entry_t *entries, size_t num_entries,
                            int *value, bool(*write_predicate)(int)) {
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
    attrs->dropdown_int.write_predicate = write_predicate;
    lv_obj_add_event_cb(dropdown, pref_dropdown_int_change_cb, LV_EVENT_VALUE_CHANGED, attrs);
    lv_obj_add_event_cb(dropdown, pref_attrs_free, LV_EVENT_DELETE, attrs);
    pref_dropdown_key_hack(dropdown);
    return dropdown;
}

lv_obj_t *pref_dropdown_int_pair(lv_obj_t *parent, const pref_dropdown_int_pair_entry_t *entries, size_t num_entries,
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
    pref_dropdown_key_hack(dropdown);
    return dropdown;
}

lv_obj_t *pref_dropdown_string(lv_obj_t *parent, const pref_dropdown_string_entry_t *entries, size_t num_entries,
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
    pref_dropdown_key_hack(dropdown);
    return dropdown;
}

lv_obj_t *pref_slider(lv_obj_t *parent, int *value, int min, int max, int step) {
    lv_obj_t *slider = lv_slider_create(parent);
    lv_slider_set_range(slider, min / step, max / step);
    lv_slider_set_value(slider, *value / step, LV_ANIM_OFF);

    pref_attrs_t *attrs = lv_mem_alloc(sizeof(pref_attrs_t));
    attrs->slider.ref = value;
    attrs->slider.step = step;
    lv_obj_add_event_cb(slider, pref_slider_value_write_back, LV_EVENT_PRESSING, attrs);
    lv_obj_add_event_cb(slider, pref_slider_value_write_back, LV_EVENT_KEY, attrs);
    lv_obj_add_event_cb(slider, pref_attrs_free, LV_EVENT_DELETE, attrs);
    return slider;
}

lv_obj_t *pref_title_label(lv_obj_t *parent, const char *title) {
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, title);
    lv_obj_set_size(label, LV_PCT(100), LV_SIZE_CONTENT);
    return label;
}

lv_obj_t *pref_desc_label(lv_obj_t *parent, const char *title, bool focusable) {
    lv_obj_t *label;
    if (focusable) {
        label = lv_obj_class_create_obj(&pref_label_cls, parent);
        lv_obj_class_init_obj(label);
    } else {
        label = lv_label_create(parent);
    }
    lv_obj_set_width(label, LV_PCT(100));
    lv_obj_add_flag(label, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_set_style_pad_left(label, LV_DPX(30), 0);
    lv_obj_set_style_text_font(label, lv_theme_get_font_small(parent), 0);
    lv_obj_set_style_outline_opa(label, LV_OPA_50, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(label, lv_theme_get_color_primary(label), LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(label, LV_DPX(3), LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(label, LV_DPX(3), LV_STATE_FOCUS_KEY);
    lv_obj_set_style_radius(label, LV_DPX(4), 0);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    if (title) {
        lv_label_set_text(label, title);
    }
    return label;
}

lv_obj_t *pref_header(lv_obj_t *parent, const char *title) {
    lv_obj_t *header = lv_label_create(parent);
    lv_label_set_text(header, title);
    lv_obj_set_width(header, LV_PCT(100));
    lv_obj_set_style_pad_bottom(header, LV_DPX(4), 0);
    lv_obj_set_style_border_side(header, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_opa(header, LV_OPA_30, 0);
    lv_obj_set_style_border_color(header, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_set_style_border_width(header, LV_DPX(1), 0);
    return header;
}

static void pref_attrs_free(lv_event_t *event) {
    lv_mem_free(lv_event_get_user_data(event));
}

static void pref_checkable_value_write_back(lv_event_t *event) {
    pref_attrs_t *attrs = lv_event_get_user_data(event);
    bool checked = lv_obj_has_state(lv_event_get_current_target(event), LV_STATE_CHECKED);
    *attrs->checkbox.ref = checked ^ attrs->checkbox.reverse;
    lv_obj_t *target = lv_event_get_current_target(event);
    lv_event_send(target, LV_EVENT_VALUE_CHANGED, NULL);
}

static void pref_checkable_dpad_check_restore(lv_event_t *event) {
    uint32_t key = lv_event_get_key(event);
    if (key != LV_KEY_UP && key != LV_KEY_DOWN && key != LV_KEY_LEFT && key != LV_KEY_RIGHT) {
        return;
    }
    pref_attrs_t *attrs = lv_event_get_user_data(event);
    lv_obj_t *target = lv_event_get_current_target(event);
    if (*attrs->checkbox.ref ^ attrs->checkbox.reverse) {
        lv_obj_add_state(target, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(target, LV_STATE_CHECKED);
    }
}

static void pref_dropdown_int_change_cb(lv_event_t *event) {
    pref_attrs_t *attrs = lv_event_get_user_data(event);
    int index = lv_dropdown_get_selected(lv_event_get_current_target(event));
    int new_value = attrs->dropdown_int.entries[index].value;
    if (attrs->dropdown_int.write_predicate && !attrs->dropdown_int.write_predicate(new_value)) {
        return;
    }
    *attrs->dropdown_int.ref = new_value;
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
    lv_obj_t *target = lv_event_get_current_target(event);
    int value = lv_slider_get_value(target);
    if (lv_event_get_code(event) == LV_EVENT_KEY) {
        uint32_t key = lv_event_get_key(event);
        if (key == LV_KEY_UP || key == LV_KEY_DOWN) {
            // TODO: make this a patch to LVGL
            _lv_bar_anim_t *var = &((lv_slider_t *) target)->bar.cur_value_anim;
            var->anim_state = -1;
            lv_anim_del(var, NULL);
            lv_slider_set_value(target, *attrs->slider.ref / attrs->slider.step, LV_ANIM_OFF);
            return;
        }
    }
    *attrs->slider.ref = value * attrs->slider.step;
    lv_event_send(target, LV_EVENT_VALUE_CHANGED, NULL);
}

static void pref_dropdown_key_hack_cb(lv_event_t *event) {
    lv_obj_t *target = lv_event_get_current_target(event);
    pref_dropdown_key_hack_state_t *state = lv_event_get_user_data(event);
    if (lv_event_get_code(event) == LV_EVENT_RELEASED) {
        state->opened = lv_dropdown_is_open(target);
        return;
    }
    switch (lv_event_get_key(event)) {
        case LV_KEY_ENTER: {
            state->opened = lv_dropdown_is_open(target);
            return;
        }
        case LV_KEY_UP:
        case LV_KEY_DOWN:
        case LV_KEY_LEFT:
        case LV_KEY_RIGHT: {
            if (!state->opened && lv_dropdown_is_open(target)) {
                lv_dropdown_close(target);
            }
            lv_event_stop_processing(event);
            break;
        }
    }
}

static void pref_dropdown_key_hack_del_cb(lv_event_t *event) {
    lv_mem_free(lv_event_get_user_data(event));
}

static void pref_dropdown_key_hack(lv_obj_t *dropdown) {
    pref_dropdown_key_hack_state_t *state = lv_mem_alloc(sizeof(pref_dropdown_key_hack_state_t));
    lv_memset_00(state, sizeof(pref_dropdown_key_hack_state_t));
    lv_obj_add_event_cb(dropdown, pref_dropdown_key_hack_cb, LV_EVENT_RELEASED, state);
    lv_obj_add_event_cb(dropdown, pref_dropdown_key_hack_cb, LV_EVENT_KEY, state);
    lv_obj_add_event_cb(dropdown, pref_dropdown_key_hack_del_cb, LV_EVENT_DELETE, state);
}