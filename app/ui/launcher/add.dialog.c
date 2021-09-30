//
// Created by Mariotaku on 2021/09/17.
//

#include "add.dialog.h"
#include "backend/pcmanager.h"
#include <app.h>
#include <errors.h>

#ifdef __WIN32

#include <winsock2.h>
#include <ws2tcpip.h>

#else

#include <arpa/inet.h>

#endif

static lv_obj_t *create_dialog(lv_obj_controller_t *self, lv_obj_t *parent);

static void dialog_cb(lv_event_t *event);

static void input_changed_cb(lv_event_t *event);

static void input_key_cb(lv_event_t *event);

static void add_cb(const pcmanager_resp_t *resp, void *userdata);

typedef struct add_dialog_controller_t {
    lv_obj_controller_t base;
    lv_obj_t *input;
    lv_obj_t *progress;
    lv_obj_t *btns;
    lv_obj_t *error;
} add_dialog_controller_t;

const lv_obj_controller_class_t add_dialog_class = {
        .create_obj_cb = create_dialog,
        .instance_size = sizeof(add_dialog_controller_t),
};

static lv_obj_t *create_dialog(lv_obj_controller_t *self, lv_obj_t *parent) {
    LV_UNUSED(parent);
    add_dialog_controller_t *controller = (add_dialog_controller_t *) self;
    const static char *btn_texts[] = {"Cancel", "OK", ""};
    lv_obj_t *dialog = lv_msgbox_create(NULL, NULL, NULL, btn_texts, false);
    lv_obj_add_event_cb(dialog, dialog_cb, LV_EVENT_VALUE_CHANGED, controller);
    lv_obj_t *content = lv_msgbox_get_content(dialog);
    lv_obj_set_style_pad_all(content, lv_dpx(8), 0);
    lv_obj_set_style_pad_gap(content, lv_dpx(8), 0);
    lv_obj_set_layout(content, LV_LAYOUT_GRID);
    static lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_CONTENT, 1, LV_GRID_TEMPLATE_LAST};
    static const lv_coord_t row_dsc[] = {LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
    col_dsc[2] = LV_DPX(10);
    lv_obj_set_grid_dsc_array(content, col_dsc, row_dsc);

    lv_obj_t *ip_label = lv_label_create(content);
    lv_obj_set_grid_cell(ip_label, LV_GRID_ALIGN_START, 0, 3, LV_GRID_ALIGN_STRETCH, 0, 1);
    lv_obj_set_height(ip_label, LV_SIZE_CONTENT);
    lv_label_set_text_static(ip_label, "IP address");

    lv_obj_t *ip_input = lv_textarea_create(content);
    lv_obj_set_grid_cell(ip_input, LV_GRID_ALIGN_STRETCH, 0, 3, LV_GRID_ALIGN_STRETCH, 1, 1);
    lv_textarea_set_placeholder_text(ip_input, "IPv4 address only");
    lv_textarea_set_one_line(ip_input, true);
    lv_textarea_set_accepted_chars(ip_input, ".-_0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
    lv_obj_add_event_cb(ip_input, input_changed_cb, LV_EVENT_VALUE_CHANGED, controller);
    lv_obj_add_event_cb(ip_input, input_key_cb, LV_EVENT_KEY, controller);

    lv_obj_t *add_progress = lv_spinner_create(content, 1000, 60);
    lv_obj_add_flag(add_progress, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_size(add_progress, lv_dpx(25), lv_dpx(25));
    lv_obj_set_grid_cell(add_progress, LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_CENTER, 1, 1);
    lv_obj_set_style_arc_width(add_progress, lv_dpx(5), 0);
    lv_obj_set_style_arc_width(add_progress, lv_dpx(5), LV_PART_INDICATOR);

    lv_obj_t *add_error = lv_label_create(content);
    lv_obj_add_flag(add_error, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_height(add_error, LV_SIZE_CONTENT);
    lv_obj_set_grid_cell(add_error, LV_GRID_ALIGN_START, 0, 3, LV_GRID_ALIGN_STRETCH, 2, 1);
    lv_label_set_long_mode(add_error, LV_LABEL_LONG_WRAP);
    lv_label_set_text_static(add_error, "Failed to add computer.");

    lv_obj_t *btns = lv_msgbox_get_btns(dialog);
    lv_btnmatrix_set_btn_ctrl(btns, 1, LV_BTNMATRIX_CTRL_DISABLED);

    controller->progress = add_progress;
    controller->input = ip_input;
    controller->btns = btns;
    controller->error = add_error;
    lv_obj_center(dialog);
    return dialog;
}

static void dialog_cb(lv_event_t *event) {
    add_dialog_controller_t *controller = lv_event_get_user_data(event);
    lv_obj_t *dialog = lv_event_get_current_target(event);
    if (dialog != controller->base.obj) return;
    uint16_t btn = lv_msgbox_get_active_btn(dialog);
    if (btn == 1) {
        const char *address = lv_textarea_get_text(controller->input);
        if (!address || !address[0])return;
        lv_obj_add_state(controller->btns, LV_STATE_DISABLED);
        lv_obj_add_state(controller->input, LV_STATE_DISABLED);
        lv_obj_clear_flag(controller->progress, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(controller->error, LV_OBJ_FLAG_HIDDEN);
        pcmanager_manual_add(pcmanager, address, add_cb, controller);
    } else {
        lv_msgbox_close_async(dialog);
    }
}

static void input_changed_cb(lv_event_t *event) {
    add_dialog_controller_t *controller = lv_event_get_user_data(event);
    const char *address = lv_textarea_get_text(controller->input);
    if (address && address[0]) {
        lv_btnmatrix_clear_btn_ctrl(controller->btns, 1, LV_BTNMATRIX_CTRL_DISABLED);
    } else {
        lv_btnmatrix_set_btn_ctrl(controller->btns, 1, LV_BTNMATRIX_CTRL_DISABLED);
    }
}

static void input_key_cb(lv_event_t *event) {
    add_dialog_controller_t *controller = lv_event_get_user_data(event);
    switch (lv_event_get_key(event)) {
        case LV_KEY_UP: {
            lv_group_t *group = lv_obj_get_child_group(controller->base.obj);
            lv_group_focus_prev(group);
            break;
        }
        case LV_KEY_DOWN: {
            lv_group_t *group = lv_obj_get_child_group(controller->base.obj);
            lv_group_focus_next(group);
            break;
        }
        case LV_KEY_ESC: {
            lv_obj_t *target = lv_event_get_current_target(event);
            if (lv_obj_has_state(target, LV_EVENT_FOCUSED)) {
                lv_event_send(target, LV_EVENT_DEFOCUSED, NULL);
            } else {
                lv_msgbox_close_async(controller->base.obj);
            }
            break;
        }
    }
}

static void add_cb(const pcmanager_resp_t *resp, void *userdata) {
    add_dialog_controller_t *controller = userdata;
    lv_obj_t *btns = lv_msgbox_get_btns(controller->base.obj);
    lv_obj_clear_state(btns, LV_STATE_DISABLED);
    lv_obj_clear_state(controller->input, LV_STATE_DISABLED);
    lv_obj_add_flag(controller->progress, LV_OBJ_FLAG_HIDDEN);
    if (resp->result.code == GS_OK) {
        lv_msgbox_close_async(controller->base.obj);
    } else {
        lv_obj_clear_flag(controller->error, LV_OBJ_FLAG_HIDDEN);
    }
}
