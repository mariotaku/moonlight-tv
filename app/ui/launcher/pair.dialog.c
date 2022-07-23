#include "app.h"
#include "launcher.controller.h"
#include "lvgl/util/lv_app_utils.h"

#include "errors.h"
#include "util/i18n.h"
#include "backend/pcmanager/priv.h"
#include "ui/root.h"

typedef struct {
    lv_fragment_t base;
    char *uuid;
    char pin[8];

    lv_obj_t *message_label;
    lv_obj_t *pin_label;
    lv_obj_t *btns;
} pair_dialog_controller_t;

static void pair_controller_ctor(lv_fragment_t *self, void *args);

static void pair_controller_dtor(lv_fragment_t *self);

static lv_obj_t *pair_dialog(lv_fragment_t *self, lv_obj_t *parent);

static void pair_result_cb(const pcmanager_resp_t *resp, void *userdata);

static void dialog_cb(lv_event_t *event);

const lv_fragment_class_t pair_dialog_class = {
        .constructor_cb = pair_controller_ctor,
        .destructor_cb = pair_controller_dtor,
        .create_obj_cb = pair_dialog,
        .instance_size = sizeof(pair_dialog_controller_t),
};

void pair_dialog_open(const SERVER_LIST *node) {
    lv_fragment_t *fragment = lv_fragment_create(&pair_dialog_class, (void *) node);
    lv_obj_t *msgbox = lv_fragment_create_obj(fragment, NULL);
    lv_obj_add_event_cb(msgbox, ui_cb_destroy_fragment, LV_EVENT_DELETE, fragment);
}

static void pair_controller_ctor(lv_fragment_t *self, void *args) {
    pair_dialog_controller_t *controller = (pair_dialog_controller_t *) self;
    controller->uuid = strdup(((const SERVER_LIST *) args)->server->uuid);
}

static void pair_controller_dtor(lv_fragment_t *self) {
    pair_dialog_controller_t *controller = (pair_dialog_controller_t *) self;
    free(controller->uuid);
}

static lv_obj_t *pair_dialog(lv_fragment_t *self, lv_obj_t *parent) {
    pair_dialog_controller_t *controller = (pair_dialog_controller_t *) self;
    static const char *btn_texts[] = {translatable("OK"), ""};
    lv_obj_t *dialog = lv_msgbox_create_i18n(NULL, locstr("Pairing"), NULL, btn_texts, false);

    controller->btns = lv_msgbox_get_btns(dialog);
    lv_obj_add_flag(controller->btns, LV_OBJ_FLAG_HIDDEN);

    PSERVER_LIST node = pcmanager_find_by_uuid(pcmanager, controller->uuid);
    if (!node || !pcmanager_pair(pcmanager, node->server, controller->pin, pair_result_cb, controller)) {
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
    lv_label_set_text(message_label, locstr("Enter PIN code on your computer."));
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
        launcher_controller_t *launcher_controller = launcher_instance();
        if (launcher_controller) {
            launcher_select_server(launcher_controller, controller->uuid);
        }
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