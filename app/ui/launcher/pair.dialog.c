//
// Created by Mariotaku on 2021/09/17.
//

#include <app.h>
#include <errors.h>
#include "pair.dialog.h"
#include "backend/pcmanager.h"

typedef struct {
    lv_obj_controller_t base;
    SERVER_LIST *node;
    char pin[8];

    lv_obj_t *message_label;
    lv_obj_t *pin_label;
    lv_obj_t *btns;
} pair_dialog_controller_t;

static void pair_controller_ctor(lv_obj_controller_t *self, void *args);

static lv_obj_t *pair_dialog(lv_obj_controller_t *self, lv_obj_t *parent);

static void pair_result_cb(const pcmanager_resp_t *resp, void *userdata);

static void dialog_cb(lv_event_t *event);

const lv_obj_controller_class_t pair_dialog_class = {
        .constructor_cb = pair_controller_ctor,
        .create_obj_cb = pair_dialog,
        .instance_size = sizeof(pair_dialog_controller_t),
};

void pair_controller_ctor(lv_obj_controller_t *self, void *args) {
    pair_dialog_controller_t *controller = (pair_dialog_controller_t *) self;
    controller->node = args;
}

static lv_obj_t *pair_dialog(lv_obj_controller_t *self, lv_obj_t *parent) {
    pair_dialog_controller_t *controller = (pair_dialog_controller_t *) self;
    static const char *btn_texts[] = {"OK", ""};
    lv_obj_t *dialog = lv_msgbox_create(NULL, "Pairing", NULL, btn_texts, false);

    controller->btns = lv_msgbox_get_btns(dialog);
    lv_obj_add_flag(controller->btns, LV_OBJ_FLAG_HIDDEN);

    if (!pcmanager_pair(pcmanager, controller->node->server, controller->pin, pair_result_cb, controller)) {
        lv_msgbox_close_async(dialog);
        return dialog;
    }
    lv_obj_t *content = lv_msgbox_get_content(dialog);
    lv_obj_set_layout(content, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *message_label = lv_label_create(content);
    lv_obj_set_size(message_label, LV_PCT(100), LV_SIZE_CONTENT);
    lv_label_set_long_mode(message_label, LV_LABEL_LONG_WRAP);
    lv_label_set_text(message_label, "Enter PIN code on your computer.");
    controller->message_label = message_label;

    lv_obj_t *pin_label = lv_label_create(content);
    lv_obj_set_style_text_font(pin_label, &lv_font_montserrat_48, 0);
    lv_obj_set_style_pad_top(pin_label, lv_dpx(15), 0);
    lv_label_set_text(pin_label, controller->pin);
    controller->pin_label = pin_label;

    lv_obj_add_event_cb(dialog, dialog_cb, LV_EVENT_VALUE_CHANGED, controller);
    lv_obj_center(dialog);
    return dialog;
}

static void pair_result_cb(const pcmanager_resp_t *resp, void *userdata) {
    pair_dialog_controller_t *controller = (pair_dialog_controller_t *) userdata;
    if (resp->result.code == GS_OK) {
        pcmanager_request_update(pcmanager, controller->node->server, NULL, NULL);
        lv_msgbox_close_async(controller->base.obj);
        return;
    }
    lv_obj_clear_flag(controller->btns, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(controller->message_label, resp->result.error.message);
    lv_obj_add_flag(controller->pin_label, LV_OBJ_FLAG_HIDDEN);
}

static void dialog_cb(lv_event_t *event) {
    pair_dialog_controller_t *controller = lv_event_get_user_data(event);
    lv_obj_t *dialog = lv_event_get_current_target(event);
    if (dialog != controller->base.obj) return;
    lv_msgbox_close_async(dialog);
}