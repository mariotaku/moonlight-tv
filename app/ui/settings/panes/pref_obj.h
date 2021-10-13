#pragma once

#include <lvgl.h>

typedef struct pref_dropdown_int_entry_t {
    const char *name;
    int value;
    bool fallback;
} pref_dropdown_int_entry_t;

typedef struct pref_dropdown_int_pair_entry_t {
    const char *name;
    int value_a, value_b;
    bool fallback;
} pref_dropdown_int_pair_entry_t;

typedef struct pref_dropdown_string_entry_t {
    const char *name;
    const char *value;
    bool fallback;
} pref_dropdown_string_entry_t;

lv_obj_t *pref_checkbox(lv_obj_t *parent, const char *title, bool *value, bool reverse);

lv_obj_t *pref_dropdown_int(lv_obj_t *parent, const pref_dropdown_int_entry_t *entries, size_t num_entries, int *value);

lv_obj_t *pref_dropdown_int_pair(lv_obj_t *parent, const pref_dropdown_int_pair_entry_t *entries, size_t num_entries,
                                 int *value_a, int *value_b);

lv_obj_t *pref_dropdown_string(lv_obj_t *parent, const pref_dropdown_string_entry_t *entries, size_t num_entries,
                               char **value);

lv_obj_t *pref_slider(lv_obj_t *parent, int *value, int min, int max, int step);

lv_obj_t *pref_title_label(lv_obj_t *parent, const char *title);

lv_obj_t *pref_desc_label(lv_obj_t *parent, const char *title);