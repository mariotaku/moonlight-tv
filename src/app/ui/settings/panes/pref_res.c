#include "pref_res.h"

#include "lvgl/util/lv_app_utils.h"

#include "util/i18n.h"

typedef struct resolution_pair {
    int w;
    int h;
    bool fallback;
} resolution_pair_t;

typedef struct {
    lv_obj_t *dropdown;
    lv_obj_t *dialog;

    pref_dropdown_int_pair_entry_t *entries;
    int num_entries;

    int max_w, max_h; // Maximum allowed resolution dimensions

    int *w_ref, *h_ref;

    int editing_w, editing_h; // Used for custom resolution input

    char custom_res_text[32];
    char placeholder_w[16];
    char placeholder_h[16];
    uint16_t selected_index;
    bool cus_value_set;

    bool change_ev_received;

    bool input_reached_start, input_reached_end;
} pref_resolution_ctx_t;

static const resolution_pair_t built_in_resolutions[] = {
    {1280, 720, true},
    {1920, 1080},
    {2560, 1440},
    {3840, 2160},
};

static const int num_built_in_resolutions = sizeof(built_in_resolutions) / sizeof(resolution_pair_t);

static void dropdown_value_changed_cb(lv_event_t *e);

static void dropdown_key_rel_cb(lv_event_t *e);

static void dropdown_delete_cb(lv_event_t *e);

static void res_input_change_cb(lv_event_t *e);

static void res_input_key_cb(lv_event_t *e);

static void cus_res_dialog_cb(lv_event_t *e);

static void populate_dialog_btn(pref_resolution_ctx_t *ctx);

static bool should_write_resolution(lv_obj_t *dropdown, int w, int h) {
    (void) w;
    (void) h;
    uint16_t option_cnt = lv_dropdown_get_option_cnt(dropdown);
    // Don't save the resolution if the user selects "Custom Resolution" option
    return lv_dropdown_get_selected(dropdown) != option_cnt - 1;
}

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

    pref_resolution_ctx_t *ctx = lv_mem_alloc(sizeof(pref_resolution_ctx_t));
    lv_memset_00(ctx, sizeof(pref_resolution_ctx_t));

    strncpy(label_buf, locstr("Custom Resolution"), sizeof(label_buf));
    entries[num_entries].name = strdup(label_buf);
    if (using_custom) {
        entries[num_entries].value_a = *w_value; // Custom resolution will be set by the user
        entries[num_entries].value_b = *h_value;
    } else {
        entries[num_entries].value_a = 0; // Indicate that the user should set a custom resolution
        entries[num_entries].value_b = 0;
    }
    entries[num_entries].fallback = false;
    num_entries++;

    ctx->entries = entries;
    ctx->num_entries = num_entries;
    ctx->max_w = max_w;
    ctx->max_h = max_h;
    ctx->w_ref = w_value;
    ctx->h_ref = h_value;

    lv_obj_t *dropdown = pref_dropdown_int_pair(parent, entries, num_entries, w_value, h_value,
                                                should_write_resolution);
    ctx->dropdown = dropdown;
    ctx->selected_index = lv_dropdown_get_selected(dropdown);
    if (using_custom) {
        snprintf(ctx->custom_res_text, sizeof(ctx->custom_res_text), locstr("%d * %d (Custom)"), *w_value, *h_value);
        lv_dropdown_set_text(dropdown, ctx->custom_res_text);
    }

    lv_obj_add_event_cb(dropdown, dropdown_value_changed_cb, LV_EVENT_VALUE_CHANGED, ctx);
    lv_obj_add_event_cb(dropdown, dropdown_key_rel_cb, LV_EVENT_RELEASED, ctx);
    lv_obj_add_event_cb(dropdown, dropdown_delete_cb, LV_EVENT_DELETE, ctx);
    return dropdown;
}

static void dropdown_value_changed_cb(lv_event_t *e) {
    pref_resolution_ctx_t *ctx = lv_event_get_user_data(e);
    uint16_t index = lv_dropdown_get_selected(lv_event_get_current_target(e));
    if (ctx->cus_value_set) {
        snprintf(ctx->custom_res_text, sizeof(ctx->custom_res_text), locstr("%d * %d (Custom)"), *ctx->w_ref,
                 *ctx->h_ref);
        lv_dropdown_set_text(ctx->dropdown, ctx->custom_res_text);
        ctx->cus_value_set = false;
        return;
    }
    lv_dropdown_set_text(ctx->dropdown, NULL);
    if (index != ctx->num_entries - 1) {
        // Set the value directly if it's not the custom FPS option
        ctx->selected_index = index;
        return;
    }
    ctx->change_ev_received = true;
    // Prevent the event from propagating further
    lv_event_stop_processing(e);


    const static char *btn_texts[] = {translatable("Cancel"), translatable("OK"), ""};
    lv_obj_t *msgbox = lv_msgbox_create_i18n(NULL, locstr("Custom Resolution"), NULL, btn_texts, false);
    ctx->dialog = msgbox;
    lv_obj_set_user_data(msgbox, ctx);
    lv_obj_t *content = lv_msgbox_get_content(msgbox);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);

    lv_obj_t *hint = lv_label_create(content);
    lv_obj_set_style_text_font(hint, lv_theme_get_font_small(hint), 0);
    lv_label_set_text(hint, locstr("Enter a custom resolution:"));
    lv_obj_set_style_pad_bottom(hint, LV_DPX(10), 0);

    lv_obj_t *row = lv_obj_create(content);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_SPACE_BETWEEN);
    lv_obj_set_style_pad_all(row, LV_DPX(5), 0);

    lv_obj_t *input_w = lv_textarea_create(row);
    lv_obj_set_style_text_font(input_w, lv_theme_get_font_small(input_w), 0);
    lv_textarea_set_one_line(input_w, true);
    lv_textarea_set_accepted_chars(input_w, "0123456789");
    lv_textarea_set_max_length(input_w, 4);
    snprintf(ctx->placeholder_w, sizeof(ctx->placeholder_w), "%d", ctx->editing_w);
    lv_textarea_set_placeholder_text(input_w, ctx->placeholder_w);
    lv_obj_set_user_data(input_w, ctx);
    lv_obj_set_flex_grow(input_w, 1);
    lv_obj_add_event_cb(input_w, res_input_change_cb, LV_EVENT_VALUE_CHANGED, &ctx->editing_w);
    lv_obj_add_event_cb(input_w, res_input_key_cb, LV_EVENT_KEY, ctx);

    lv_obj_t *label_multiply = lv_label_create(row);
    lv_label_set_text(label_multiply, "*");
    lv_obj_set_style_pad_hor(label_multiply, LV_DPX(10), 0);

    lv_obj_t *input_h = lv_textarea_create(row);
    lv_obj_set_style_text_font(input_h, lv_theme_get_font_small(input_h), 0);
    lv_textarea_set_one_line(input_h, true);
    lv_textarea_set_accepted_chars(input_h, "0123456789");
    lv_textarea_set_max_length(input_h, 4);
    snprintf(ctx->placeholder_h, sizeof(ctx->placeholder_h), "%d", ctx->editing_h);
    lv_textarea_set_placeholder_text(input_h, ctx->placeholder_h);
    lv_obj_set_user_data(input_h, ctx);
    lv_obj_set_flex_grow(input_h, 1);
    lv_obj_add_event_cb(input_h, res_input_change_cb, LV_EVENT_VALUE_CHANGED, &ctx->editing_h);
    lv_obj_add_event_cb(input_h, res_input_key_cb, LV_EVENT_KEY, ctx);

    lv_obj_add_event_cb(msgbox, cus_res_dialog_cb, LV_EVENT_VALUE_CHANGED, ctx);

    lv_obj_center(msgbox);

    populate_dialog_btn(ctx);
}

static void dropdown_key_rel_cb(lv_event_t *e) {
    pref_resolution_ctx_t *ctx = lv_event_get_user_data(e);
    lv_obj_t *dropdown = lv_event_get_current_target(e);
    lv_indev_t *indev = lv_indev_get_act();
    if (lv_indev_get_scroll_obj(indev) != NULL) {
        return;
    }
    if (lv_dropdown_is_open(dropdown)) {
        return;
    }
    uint16_t selected = lv_dropdown_get_selected(dropdown);
    if (selected != ctx->num_entries - 1) {
        return;
    }
    if (ctx->change_ev_received) {
        // This flag is set when user changed selection from a built-in resolution to the custom resolution option.
        return;
    }
    lv_event_send(dropdown, LV_EVENT_VALUE_CHANGED, &selected);
}

static void dropdown_delete_cb(lv_event_t *e) {
    pref_resolution_ctx_t *ctx = lv_event_get_user_data(e);
    for (int i = 0; i < ctx->num_entries; i++) {
        // The name strings are copied with strdup. We need to free them.
        free((void *) ctx->entries[i].name);
    }
    lv_mem_free(ctx->entries);

    // Free the context
    lv_mem_free(ctx);
}

static void res_input_change_cb(lv_event_t *e) {
    lv_obj_t *input = lv_event_get_current_target(e);
    pref_resolution_ctx_t *ctx = lv_obj_get_user_data(input);
    int *value_ref = (int *) lv_event_get_user_data(e);
    *value_ref = (int) strtol(lv_textarea_get_text(input), NULL, 10);

    populate_dialog_btn(ctx);
}

static void res_input_key_cb(lv_event_t *e) {
    pref_resolution_ctx_t *ctx = lv_event_get_user_data(e);
    lv_obj_t *input = lv_event_get_current_target(e);
    switch (lv_event_get_key(e)) {
        case LV_KEY_DOWN: {
            lv_group_focus_obj(lv_msgbox_get_btns(ctx->dialog));
            break;
        }
        case LV_KEY_LEFT: {
            if (lv_textarea_get_cursor_pos(input) > 0) {
                ctx->input_reached_start = false;
                ctx->input_reached_end = false;
                return;
            }
            if (!ctx->input_reached_start) {
                ctx->input_reached_start = true;
                return;
            }
            ctx->input_reached_start = false;
            lv_group_t *group = lv_obj_get_group(input);
            if (group == NULL) {
                return;
            }
            lv_group_focus_prev(group);
            break;
        }
        case LV_KEY_RIGHT: {
            if (lv_textarea_get_cursor_pos(input) < strlen(lv_textarea_get_text(input))) {
                ctx->input_reached_start = false;
                ctx->input_reached_end = false;
                return;
            }
            if (!ctx->input_reached_end) {
                ctx->input_reached_end = true;
                return;
            }
            ctx->input_reached_end = false;
            lv_group_t *group = lv_obj_get_group(input);
            if (group == NULL) {
                return;
            }
            lv_group_focus_next(group);
            break;
        }
    }
}

static void cus_res_dialog_cb(lv_event_t *e) {
    lv_obj_t *mbox = lv_event_get_current_target(e);
    pref_resolution_ctx_t *ctx = (pref_resolution_ctx_t *) lv_obj_get_user_data(mbox);
    ctx->change_ev_received = false;
    if (lv_msgbox_get_active_btn(mbox) != 1) {
        lv_dropdown_set_selected(ctx->dropdown, ctx->selected_index);
        lv_msgbox_close_async(mbox);
        return;
    }

    *ctx->w_ref = ctx->editing_w;
    *ctx->h_ref = ctx->editing_h;
    ctx->cus_value_set = true;
    lv_event_send(ctx->dropdown, LV_EVENT_VALUE_CHANGED, NULL);
    lv_msgbox_close_async(mbox);
}

static void populate_dialog_btn(pref_resolution_ctx_t *ctx) {
    bool res_valid = true;
    if (ctx->editing_w <= 0 || ctx->editing_h <= 0 || ctx->editing_w % 8 != 0 || ctx->editing_h % 8 != 0) {
        // width or height is not valid
        res_valid = false;
    } else if (ctx->editing_w > ctx->max_w || ctx->editing_h > ctx->max_h) {
        // width or height exceeds the maximum allowed dimensions
        res_valid = false;
    }
    lv_obj_t *dialog_btns = lv_msgbox_get_btns(ctx->dialog);
    if (res_valid) {
        lv_btnmatrix_clear_btn_ctrl(dialog_btns, 1, LV_BTNMATRIX_CTRL_DISABLED);
    } else {
        lv_btnmatrix_set_btn_ctrl(dialog_btns, 1, LV_BTNMATRIX_CTRL_DISABLED);
    }
}

