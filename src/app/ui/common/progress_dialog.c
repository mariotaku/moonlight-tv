#include "progress_dialog.h"

lv_obj_t *progress_dialog_create(const char *message) {
    lv_obj_t *dialog = lv_msgbox_create(NULL, NULL, NULL, NULL, false);
    lv_obj_t *content = lv_msgbox_get_content(dialog);
    lv_obj_set_layout(content, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t *spinner = lv_spinner_create(content, 1000, 60);
    lv_obj_set_style_arc_width(spinner, lv_dpx(10), 0);
    lv_obj_set_style_arc_width(spinner, lv_dpx(10), LV_PART_INDICATOR);
    lv_obj_set_size(spinner, lv_dpx(50), lv_dpx(50));
    lv_obj_set_flex_grow(spinner, 0);
    lv_obj_t *label = lv_label_create(content);
    lv_obj_set_style_pad_hor(label, LV_DPX(20), 0);
    lv_label_set_text(label, message);
    lv_obj_set_flex_grow(label, 1);

    lv_obj_center(dialog);
    return dialog;
}