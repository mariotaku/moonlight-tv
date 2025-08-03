#include "pref_res.h"
#include "util/i18n.h"

typedef struct resolution_pair {
    int w;
    int h;
    bool fallback;
} resolution_pair_t;

static const resolution_pair_t built_in_resolutions[] = {
    {1280, 720, true},
    {1920, 1080},
    {2560, 1440},
    {3840, 2160}
};

static const int num_built_in_resolutions = sizeof(built_in_resolutions) / sizeof(resolution_pair_t);

lv_obj_t *pref_dropdown_res(lv_obj_t *parent, int native_w, int native_h, int max_w, int max_h,
                            int *w_value, int *h_value) {
    bool is_16by9 = native_w <= 0 || native_h <= 0 || native_w * 9 == native_h * 16;
    bool using_custom = true;

    // maximum 4 built-in resolutions + 1 native resolution + 1 custom resolution
    pref_dropdown_int_pair_entry_t *entries = lv_mem_alloc(6 * sizeof(pref_dropdown_int_pair_entry_t));
    int num_entries = 0;

    char label_buf[32];
    for (int i = 0; i < num_built_in_resolutions; i++) {
        int builtin_w = built_in_resolutions[i].w, builtin_h = built_in_resolutions[i].h;
        if ((max_w > 0 && builtin_w > max_w) || (max_h > 0 && builtin_h > max_h)) {
            continue; // Skip resolutions that exceed the maximum dimensions
        }
        if (using_custom && builtin_w == *w_value && builtin_h == *h_value) {
            using_custom = false; // If the current values match a built-in resolution, we're not using custom
        }

        if (builtin_w == native_w && builtin_h == native_h) {
            snprintf(label_buf, sizeof(label_buf), "%d * %d (Native)", builtin_w, builtin_h);
        } else {
            snprintf(label_buf, sizeof(label_buf), "%d * %d", builtin_w, builtin_h);
        }
        entries[num_entries].name = strdup(label_buf);
        entries[num_entries].value_a = builtin_w;
        entries[num_entries].value_b = builtin_h;
        entries[num_entries].fallback = built_in_resolutions[i].fallback;
        num_entries++;
    }
    if (!is_16by9) {
        if (using_custom && *w_value == native_w && *h_value == native_h) {
            using_custom = false; // If the current values match the native resolution, we're not using custom
        }

        snprintf(label_buf, sizeof(label_buf), "%d * %d (Native)", native_w, native_h);
        entries[num_entries].name = strdup(label_buf);
        entries[num_entries].value_a = native_w;
        entries[num_entries].value_b = native_h;
        entries[num_entries].fallback = false;
        num_entries++;
    }

    strncpy(label_buf, locstr("Custom Resolution"), sizeof(label_buf));
    entries[num_entries].name = strdup(label_buf);
    entries[num_entries].value_a = 0; // Custom resolution will be set by the user
    entries[num_entries].value_b = 0;
    entries[num_entries].fallback = false;
    num_entries++;

    lv_obj_t *dropdown = pref_dropdown_int_pair(parent, entries, num_entries, w_value, h_value);
    return dropdown;
}