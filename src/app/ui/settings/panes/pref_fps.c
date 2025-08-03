#include "pref_fps.h"

#include "lvgl/util/lv_app_utils.h"

#include "util/i18n.h"

static bool is_valid_fps(int fps) {
    return fps > 0;
}

static void dropdown_fps_select_cb(lv_event_t *e);

static void dropdown_delete_cb(lv_event_t *e);

static void cus_fps_dialog_cb(lv_event_t *e);

static void cus_slider_change_cb(lv_event_t *e);

static void cus_slider_key_cb(lv_event_t *e);

typedef struct pref_dropdown_fps_ctx {
    lv_obj_t *dropdown;
    pref_dropdown_int_entry_t *entries;
    int num_entries;
    int *value_ref;
    char custom_fps_text[32];
    uint16_t selected_index;
    bool cus_value_set;
    int cus_slider_value;
} pref_dropdown_fps_ctx_t;

lv_obj_t *pref_dropdown_fps(lv_obj_t *parent, const int *options, int max, int *value) {
    bool has_custom_fps = true;
    int num_entries;
    for (num_entries = 0; options[num_entries]; num_entries++) {
        if (options[num_entries] == *value) {
            // If the current value is in the options, then it's not a custom FPS
            has_custom_fps = false;
        }
        if (options[num_entries] > max) {
            break;
        }
    }
    // Include an extra entry for the custom FPS option
    num_entries++;

    pref_dropdown_int_entry_t *entries = lv_mem_alloc(sizeof(pref_dropdown_int_entry_t) * num_entries);
    char buf[32];
    for (int i = 0; i < num_entries; i++) {
        snprintf(buf, sizeof(buf), locstr("%d FPS"), options[i]);
        entries[i].name = strndup(buf, sizeof(buf));
        entries[i].value = options[i];
    }
    // Custom FPS option
    entries[num_entries - 1].name = locstr("Custom FPS");
    entries[num_entries - 1].value = 0;
    entries[num_entries - 1].fallback = true;

    lv_obj_t *fps_dropdown = pref_dropdown_int(parent, entries, num_entries, value, is_valid_fps);

    pref_dropdown_fps_ctx_t *ctx = lv_mem_alloc(sizeof(pref_dropdown_fps_ctx_t));
    lv_memset_00(ctx, sizeof(pref_dropdown_fps_ctx_t));
    ctx->entries = entries;
    ctx->num_entries = num_entries;
    ctx->value_ref = value;
    ctx->dropdown = fps_dropdown;
    ctx->selected_index = lv_dropdown_get_selected(fps_dropdown);

    if (has_custom_fps) {
        snprintf(ctx->custom_fps_text, sizeof(ctx->custom_fps_text), locstr("%d FPS (Custom)"), *value);
        lv_dropdown_set_text(fps_dropdown, ctx->custom_fps_text);
    }

    lv_obj_add_event_cb(fps_dropdown, dropdown_fps_select_cb, LV_EVENT_VALUE_CHANGED, ctx);
    lv_obj_add_event_cb(fps_dropdown, dropdown_delete_cb, LV_EVENT_DELETE, ctx);
    return fps_dropdown;
}


void dropdown_fps_select_cb(lv_event_t *e) {
    pref_dropdown_fps_ctx_t *ctx = (pref_dropdown_fps_ctx_t *) lv_event_get_user_data(e);
    uint16_t index = lv_dropdown_get_selected(lv_event_get_current_target(e));
    if (ctx->cus_value_set) {
        snprintf(ctx->custom_fps_text, sizeof(ctx->custom_fps_text), locstr("%d FPS (Custom)"), *ctx->value_ref);
        lv_dropdown_set_text(ctx->dropdown, ctx->custom_fps_text);
        ctx->cus_value_set = false;
        return;
    }
    lv_dropdown_set_text(ctx->dropdown, NULL);
    if (index != ctx->num_entries - 1) {
        // Set the value directly if it's not the custom FPS option
        return;
    }
    // Prevent the event from propagating further
    lv_event_stop_processing(e);

    const static char *btn_texts[] = {translatable("Cancel"), translatable("OK"), ""};
    lv_obj_t *msgbox = lv_msgbox_create_i18n(NULL, locstr("Custom FPS"), NULL, btn_texts, false);
    lv_obj_set_user_data(msgbox, ctx);
    lv_obj_t *content = lv_msgbox_get_content(msgbox);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *row = lv_obj_create(content);
    lv_obj_remove_style_all(row);
    lv_obj_set_width(row, LV_PCT(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_ver(row, LV_DPX(10), 0);

    int min_fps = 30;
    int max_fps = ctx->entries[ctx->num_entries - 2].value;

    lv_obj_t *min_label = lv_label_create(row);
    lv_label_set_text_fmt(min_label, "%d", min_fps);
    lv_obj_set_style_text_align(min_label, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_min_width(min_label, LV_DPX(40), 0);

    ctx->cus_slider_value = *ctx->value_ref;
    lv_obj_t *slider = pref_slider(row, &ctx->cus_slider_value, min_fps, max_fps, 5);
    lv_obj_set_user_data(slider, ctx);
    lv_obj_set_width(slider, LV_PCT(100));
    lv_obj_set_flex_grow(slider, 1);
    lv_obj_add_event_cb(slider, cus_slider_key_cb, LV_EVENT_KEY, ctx);

    lv_obj_t *max_label = lv_label_create(row);
    lv_label_set_text_fmt(max_label, "%d", max_fps);
    lv_obj_set_style_text_align(max_label, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_min_width(max_label, LV_DPX(40), 0);

    lv_obj_t *fps_label = lv_label_create(content);
    lv_label_set_text_fmt(fps_label, locstr("%d FPS"), *ctx->value_ref);
    lv_obj_set_style_text_align(fps_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_user_data(fps_label, ctx);

    lv_obj_add_event_cb(slider, cus_slider_change_cb, LV_EVENT_VALUE_CHANGED, fps_label);

    lv_obj_add_event_cb(msgbox, cus_fps_dialog_cb, LV_EVENT_VALUE_CHANGED, slider);

    lv_obj_center(msgbox);
}

void dropdown_delete_cb(lv_event_t *e) {
    pref_dropdown_fps_ctx_t *ctx = (pref_dropdown_fps_ctx_t *) lv_event_get_user_data(e);
    for (int i = 0; i < ctx->num_entries; i++) {
        free((void *) ctx->entries[i].name);
    }
    lv_mem_free(ctx->entries);
    lv_mem_free(ctx);
}

static void cus_fps_dialog_cb(lv_event_t *e) {
    lv_obj_t *mbox = lv_event_get_current_target(e);
    pref_dropdown_fps_ctx_t *ctx = (pref_dropdown_fps_ctx_t *) lv_obj_get_user_data(mbox);
    if (lv_msgbox_get_active_btn(mbox) != 1) {
        lv_dropdown_set_selected(ctx->dropdown, ctx->selected_index);
        lv_msgbox_close_async(mbox);
        return;
    }

    *ctx->value_ref = ctx->cus_slider_value;
    ctx->cus_value_set = true;
    lv_event_send(ctx->dropdown, LV_EVENT_VALUE_CHANGED, NULL);
    lv_msgbox_close_async(mbox);
}

static void cus_slider_change_cb(lv_event_t *e) {
    lv_obj_t *slider = lv_event_get_current_target(e);
    pref_dropdown_fps_ctx_t *ctx = (pref_dropdown_fps_ctx_t *) lv_obj_get_user_data(slider);
    lv_obj_t *fps_label = lv_event_get_user_data(e);
    lv_label_set_text_fmt(fps_label, locstr("%d FPS"), ctx->cus_slider_value);
}

static void cus_slider_key_cb(lv_event_t *e) {
    if (lv_event_get_key(e) != LV_KEY_DOWN) {
        return;
    }
    lv_group_t *group = lv_obj_get_group(lv_event_get_target(e));
    lv_group_focus_next(group);
}
