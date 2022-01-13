#include "app.h"

#include "launcher.controller.h"
#include "server.context_menu.h"

#include "errors.h"
#include "backend/types.h"

#include "util/i18n.h"

typedef struct context_menu_t {
    lv_fragment_t base;
    PSERVER_LIST node;
} context_menu_t;

static void menu_ctor(lv_fragment_t *self, void *arg);

static lv_obj_t *create_obj(lv_fragment_t *self, lv_obj_t *parent);

static void context_menu_cancel_cb(lv_event_t *e);

static void context_menu_click_cb(lv_event_t *e);

static void test_btn_cb(lv_event_t *e);

static void unpair_btn_cb(lv_event_t *e);

static void test_callback(const pcmanager_resp_t *resp, void *userdata);

static void unpair_callback(const pcmanager_resp_t *resp, void *userdata);

const lv_fragment_class_t server_menu_class = {
        .constructor_cb = menu_ctor,
        .create_obj_cb = create_obj,
        .instance_size = sizeof(context_menu_t)
};

static void menu_ctor(lv_fragment_t *self, void *arg) {
    context_menu_t *controller = (context_menu_t *) self;
    controller->node = arg;
}

static lv_obj_t *create_obj(lv_fragment_t *self, lv_obj_t *parent) {
    context_menu_t *controller = (context_menu_t *) self;
    PSERVER_LIST node = controller->node;
    lv_obj_t *msgbox = lv_msgbox_create(NULL, node->server->hostname, NULL, NULL, false);
    lv_obj_t *content = lv_msgbox_get_content(msgbox);
    lv_obj_add_flag(content, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);

    lv_obj_add_event_cb(content, context_menu_cancel_cb, LV_EVENT_KEY, controller);
    lv_obj_add_event_cb(content, context_menu_click_cb, LV_EVENT_SHORT_CLICKED, controller);

    lv_obj_t *test_btn = lv_list_add_btn(content, NULL, locstr("Test Connection"));
    lv_obj_add_flag(test_btn, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_event_cb(test_btn, test_btn_cb, LV_EVENT_SHORT_CLICKED, controller);

    if (node->server->paired) {
        lv_obj_t *unpair_btn = lv_list_add_btn(content, NULL, locstr("Unpair"));
        lv_obj_add_flag(unpair_btn, LV_OBJ_FLAG_EVENT_BUBBLE | LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_event_cb(unpair_btn, unpair_btn_cb, LV_EVENT_SHORT_CLICKED, controller);
    }

    lv_obj_t *cancel_btn = lv_list_add_btn(content, NULL, locstr("Cancel"));
    lv_obj_add_flag(cancel_btn, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_center(msgbox);
    return msgbox;
}

static void context_menu_cancel_cb(lv_event_t *e) {
    lv_obj_t *target = lv_event_get_target(e);
    if (target->parent != lv_event_get_current_target(e)) return;
    lv_msgbox_close_async(lv_event_get_current_target(e)->parent);
}

static void context_menu_click_cb(lv_event_t *e) {
    lv_obj_t *target = lv_event_get_target(e);
    lv_obj_t *current_target = lv_event_get_current_target(e);
    if (target->parent != current_target) return;
    lv_obj_t *mbox = lv_event_get_current_target(e)->parent;
    lv_msgbox_close_async(mbox);
}

static void test_btn_cb(lv_event_t *e) {
    context_menu_t *controller = (context_menu_t *) lv_event_get_user_data(e);
    pcmanager_test(pcmanager, controller->node->server, test_callback, NULL);
}

static void unpair_btn_cb(lv_event_t *e) {
    context_menu_t *controller = (context_menu_t *) lv_event_get_user_data(e);
    pcmanager_unpair(pcmanager, controller->node->server, unpair_callback, NULL);
}

static void test_callback(const pcmanager_resp_t *resp, void *userdata) {
    if (resp->result.code != GS_OK) return;
    launcher_controller_t *launcher = launcher_instance();
    launcher_select_server(launcher, NULL);
}

static void unpair_callback(const pcmanager_resp_t *resp, void *userdata) {
    if (resp->result.code != GS_OK) return;
    launcher_controller_t *launcher = launcher_instance();
    launcher_select_server(launcher, NULL);
}