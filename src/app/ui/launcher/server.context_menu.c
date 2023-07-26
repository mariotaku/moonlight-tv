#include "app.h"

#include "launcher.controller.h"
#include "server.context_menu.h"

#include "backend/types.h"

#include "util/i18n.h"
#include "lvgl/util/lv_app_utils.h"
#include "util/nullable.h"

typedef struct context_menu_t {
    lv_fragment_t base;
    uuidstr_t uuid;
    bool single_clicked;
} context_menu_t;

static void menu_ctor(lv_fragment_t *self, void *arg);

static void menu_dtor(lv_fragment_t *self);

static lv_obj_t *create_obj(lv_fragment_t *self, lv_obj_t *parent);

static void context_menu_cancel_cb(lv_event_t *e);

static void context_menu_short_click_cb(lv_event_t *e);

static void context_menu_click_cb(lv_event_t *e);

static void open_info(const pclist_t *node);

static void forget_host(const pclist_t *node);

static void info_action_cb(lv_event_t *e);

const lv_fragment_class_t server_menu_class = {
        .constructor_cb = menu_ctor,
        .destructor_cb = menu_dtor,
        .create_obj_cb = create_obj,
        .instance_size = sizeof(context_menu_t)
};

static void menu_ctor(lv_fragment_t *self, void *arg) {
    context_menu_t *controller = (context_menu_t *) self;
    controller->uuid = *(const uuidstr_t *) arg;
}

static void menu_dtor(lv_fragment_t *self) {
    LV_UNUSED(self);
}

static lv_obj_t *create_obj(lv_fragment_t *self, lv_obj_t *parent) {
    LV_UNUSED(parent);
    context_menu_t *controller = (context_menu_t *) self;

    const pclist_t *node = pcmanager_node(pcmanager, &controller->uuid);
    if (!node) {
        lv_obj_t *empty = lv_msgbox_create(NULL, "Unknown", NULL, NULL, false);
        lv_msgbox_close_async(empty);
        return empty;
    }
    lv_obj_t *msgbox = lv_msgbox_create(NULL, node->server->hostname, NULL, NULL, false);
    lv_obj_t *content = lv_msgbox_get_content(msgbox);
    lv_obj_add_flag(content, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);

    lv_obj_add_event_cb(content, context_menu_cancel_cb, LV_EVENT_CANCEL, controller);
    lv_obj_add_event_cb(content, context_menu_short_click_cb, LV_EVENT_SHORT_CLICKED, controller);
    lv_obj_add_event_cb(content, context_menu_click_cb, LV_EVENT_CLICKED, controller);

    lv_obj_t *info_btn = lv_list_add_btn(content, NULL, locstr("Info"));
    lv_obj_add_flag(info_btn, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_user_data(info_btn, open_info);

    if (node->state.code == SERVER_STATE_OFFLINE || node->state.code == SERVER_STATE_ERROR) {
        lv_obj_t *forget_btn = lv_list_add_btn(content, NULL, locstr("Forget"));
        lv_obj_add_flag(forget_btn, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_set_user_data(forget_btn, forget_host);
    }

    lv_obj_t *cancel_btn = lv_list_add_btn(content, NULL, locstr("Cancel"));
    lv_obj_add_flag(cancel_btn, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_center(msgbox);
    return msgbox;
}

static void context_menu_cancel_cb(lv_event_t *e) {
    lv_obj_t *target = lv_event_get_target(e);
    if (target->parent != lv_event_get_current_target(e)) { return; }
    lv_msgbox_close(lv_event_get_current_target(e)->parent);
}

static void context_menu_short_click_cb(lv_event_t *e) {
    context_menu_t *controller = lv_event_get_user_data(e);
    controller->single_clicked = true;
}

static void context_menu_click_cb(lv_event_t *e) {
    lv_obj_t *target = lv_event_get_target(e);
    context_menu_t *controller = lv_event_get_user_data(e);
    if (!controller->single_clicked) { return; }
    lv_obj_t *current_target = lv_event_get_current_target(e);
    if (target->parent != current_target) { return; }
    void *target_userdata = lv_obj_get_user_data(target);
    lv_obj_t *mbox = lv_event_get_current_target(e)->parent;
    const pclist_t *node = pcmanager_node(pcmanager, &controller->uuid);
    lv_msgbox_close(mbox);
    if (!node) {
        return;
    }
    if (target_userdata == open_info) {
        open_info(node);
    } else if (target_userdata == forget_host) {
        forget_host(node);
    }
}

static void open_info(const pclist_t *node) {
    static const char *btn_txts[] = {translatable("OK"), ""};
    const SERVER_DATA *server = node->server;
    lv_obj_t *mbox = lv_msgbox_create_i18n(NULL, server->hostname, "placeholder", btn_txts, false);
    lv_obj_t *message = lv_msgbox_get_text(mbox);
    lv_label_set_text_fmt(message, locstr("IP address: %s\nGPU: %s\nSupports 4K: %s\n"
                                          "Supports HDR: %s\nHost Software Version: %s\n"
                                          "GeForce Experience: %s"),
                          server->serverInfo.address, str_null_or_empty(server->gpuType) ? "Unknown" : server->gpuType,
                          server->supports4K ? "YES" : "NO",
                          server->supportsHdr ? "YES" : "NO",
                          server->serverInfo.serverInfoAppVersion,
                          server->isGfe ? server->serverInfo.serverInfoGfeVersion : "NO");
    lv_obj_add_event_cb(mbox, info_action_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_center(mbox);
}

static void forget_host(const pclist_t *node) {
    bool selected = node->selected;
    pcmanager_forget(pcmanager, &node->id);
    if (selected) {
        launcher_fragment_t *launcher = launcher_instance();
        launcher_select_server(launcher, NULL);
    }
}

static void info_action_cb(lv_event_t *e) {
    lv_obj_t *mbox = lv_event_get_current_target(e);
    lv_msgbox_close_async(mbox);
}