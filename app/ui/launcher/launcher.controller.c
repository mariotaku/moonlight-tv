#include <ui/help/help.dialog.h>
#include <Limelight.h>
#include <PlatformCrypto.h>
#include "app.h"

#include "add.dialog.h"
#include "apps.controller.h"
#include "launcher.controller.h"
#include "pair.dialog.h"
#include "server.context_menu.h"

#include "ui/root.h"
#include "ui/settings/settings.controller.h"
#include "lvgl/font/material_icons_regular_symbols.h"
#include "lvgl/ext/lv_gridview.h"
#include "lvgl/util/lv_app_utils.h"

#include "stream/platform.h"

#include "util/font.h"
#include "util/i18n.h"
#include "util/user_event.h"
#include "util/logging.h"
#include "backend/pcmanager/priv.h"

static void launcher_controller(lv_fragment_t *self, void *args);

static void controller_dtor(lv_fragment_t *self);

static void launcher_view_init(lv_fragment_t *self, lv_obj_t *view);

static void launcher_view_destroy(lv_fragment_t *self, lv_obj_t *view);

static bool launcher_event_cb(lv_fragment_t *self, int code, void *userdata);

static void on_pc_added(const pcmanager_resp_t *resp, void *userdata);

static void on_pc_updated(const pcmanager_resp_t *resp, void *userdata);

static void on_pc_removed(const pcmanager_resp_t *resp, void *userdata);

static void update_pclist(launcher_controller_t *controller);

static void cb_pc_selected(lv_event_t *event);

static void cb_pc_longpress(lv_event_t *event);

static void cb_nav_focused(lv_event_t *event);

static void cb_nav_key(lv_event_t *event);

static void cb_detail_focused(lv_event_t *event);

static void cb_detail_cancel(lv_event_t *event);

static void open_manual_add(lv_event_t *event);

static void open_settings(lv_event_t *event);

static void open_help(lv_event_t *event);

static void select_pc(launcher_controller_t *controller, PSERVER_LIST selected, bool refocus);

static void set_detail_opened(launcher_controller_t *controller, bool opened);

static lv_obj_t *pclist_item_create(launcher_controller_t *controller, PSERVER_LIST cur);

static const char *server_item_icon(const SERVER_LIST *node);

static void pcitem_set_selected(lv_obj_t *pcitem, bool selected);

static void show_decoder_error(launcher_controller_t *controller);

static void decoder_error_cb(lv_event_t *e);

const lv_fragment_class_t launcher_controller_class = {
        .constructor_cb = launcher_controller,
        .destructor_cb = controller_dtor,
        .create_obj_cb = launcher_win_create,
        .obj_created_cb = launcher_view_init,
        .obj_will_delete_cb = launcher_view_destroy,
        .event_cb = launcher_event_cb,
        .instance_size = sizeof(launcher_controller_t),
};

static const pcmanager_listener_t pcmanager_callbacks = {
        .added = on_pc_added,
        .updated = on_pc_updated,
        .removed = on_pc_removed,
};

static launcher_controller_t *current_instance = NULL;

launcher_controller_t *launcher_instance() {
    return current_instance;
}

void launcher_select_server(launcher_controller_t *controller, const char *uuid) {
    PSERVER_LIST node = uuid ? pcmanager_find_by_uuid(pcmanager, uuid) : NULL;
    if (!node) {
        set_detail_opened(controller, false);
        select_pc(controller, NULL, false);
        return;
    }
    if (node->state.code == SERVER_STATE_NOT_PAIRED) {
        pair_dialog_open(node);
        return;
    }
    set_detail_opened(controller, true);
    if (node->selected) return;
    select_pc(controller, node, false);
}

static void launcher_controller(lv_fragment_t *self, void *args) {
    (void) args;
    launcher_controller_t *controller = (launcher_controller_t *) self;
    static const lv_style_prop_t props[] = {
            LV_STYLE_OPA, LV_STYLE_BG_OPA, LV_STYLE_TRANSLATE_X, LV_STYLE_TRANSLATE_Y, 0
    };
    lv_style_transition_dsc_init(&controller->tr_detail, props, lv_anim_path_ease_out, 300, 0, NULL);
    lv_style_transition_dsc_init(&controller->tr_nav, props, lv_anim_path_ease_out, 350, 0, NULL);

    lv_style_init(&controller->nav_host_style);
    int ico_width_def = lv_txt_get_width(MAT_SYMBOL_SETTINGS, sizeof(MAT_SYMBOL_SETTINGS), app_iconfonts.normal, 0, 0);
    int host_icon_pad = (LV_DPX(NAV_WIDTH_COLLAPSED) - ico_width_def) / 2;
    lv_style_set_pad_left(&controller->nav_host_style, host_icon_pad);
    lv_style_set_pad_gap(&controller->nav_host_style, host_icon_pad);

    lv_style_init(&controller->nav_menu_style);
    lv_style_set_border_side(&controller->nav_menu_style, LV_BORDER_SIDE_NONE);
    lv_style_set_pad_ver(&controller->nav_menu_style, LV_DPX(10));
    int ico_width_sm = lv_txt_get_width(MAT_SYMBOL_SETTINGS, sizeof(MAT_SYMBOL_SETTINGS), app_iconfonts.small, 0, 0);
    int nav_icon_pad = (LV_DPX(NAV_WIDTH_COLLAPSED) - ico_width_sm) / 2;
    lv_style_set_pad_left(&controller->nav_menu_style, nav_icon_pad);
    lv_style_set_pad_gap(&controller->nav_menu_style, nav_icon_pad);

    controller->detail_opened = false;
    controller->pane_initialized = false;
    controller->first_created = true;
}

static void controller_dtor(lv_fragment_t *self) {
    launcher_controller_t *controller = (launcher_controller_t *) self;
}

static void launcher_view_init(lv_fragment_t *self, lv_obj_t *view) {
    launcher_controller_t *controller = (launcher_controller_t *) self;
    pcmanager_register_listener(pcmanager, &pcmanager_callbacks, controller);
    lv_obj_add_event_cb(controller->nav, cb_nav_focused, LV_EVENT_FOCUSED, controller);
    lv_obj_add_event_cb(controller->nav, cb_nav_key, LV_EVENT_KEY, controller);
    lv_obj_add_event_cb(controller->nav, app_quit_confirm, LV_EVENT_CANCEL, controller);
    lv_obj_add_event_cb(controller->detail, cb_detail_focused, LV_EVENT_FOCUSED, controller);
    lv_obj_add_event_cb(controller->detail, cb_detail_cancel, LV_EVENT_CANCEL, controller);
    lv_obj_add_event_cb(controller->pclist, cb_pc_selected, LV_EVENT_SHORT_CLICKED, controller);
    lv_obj_add_event_cb(controller->pclist, cb_pc_longpress, LV_EVENT_LONG_PRESSED, controller);
    lv_obj_add_event_cb(controller->add_btn, open_manual_add, LV_EVENT_CLICKED, controller);
    lv_obj_add_event_cb(controller->pref_btn, open_settings, LV_EVENT_CLICKED, controller);
    lv_obj_add_event_cb(controller->help_btn, open_help, LV_EVENT_CLICKED, controller);
    lv_obj_add_event_cb(controller->quit_btn, app_quit_confirm, LV_EVENT_CLICKED, controller);

    update_pclist(controller);

    for (PSERVER_LIST cur = pcmanager_servers(pcmanager); cur != NULL; cur = cur->next) {
        if (cur->selected) {
            select_pc(controller, cur, true);
            if (controller->first_created) {
                controller->detail_opened = true;
            }
            continue;
        }
        pcmanager_request_update(pcmanager, cur->server, NULL, NULL);
    }
    controller->pane_initialized = true;
    set_detail_opened(controller, controller->detail_opened);
    pcmanager_auto_discovery_start(pcmanager);

    lv_obj_set_style_transition(controller->detail, &controller->tr_nav, 0);
    lv_obj_set_style_transition(controller->detail, &controller->tr_detail, LV_STATE_USER_1);
    current_instance = controller;

    if (decoder_current == DECODER_EMPTY && controller->first_created) {
        show_decoder_error(controller);
    }
    controller->first_created = false;
}

static void launcher_view_destroy(lv_fragment_t *self, lv_obj_t *view) {
    current_instance = NULL;
    app_input_set_group(NULL);
    pcmanager_auto_discovery_stop(pcmanager);

    launcher_controller_t *controller = (launcher_controller_t *) self;
    controller->pane_initialized = false;

    lv_group_del(controller->nav_group);
    lv_group_del(controller->detail_group);

    pcmanager_unregister_listener(pcmanager, &pcmanager_callbacks);
}

static bool launcher_event_cb(lv_fragment_t *self, int code, void *userdata) {
    LV_UNUSED(userdata);
    switch (code) {
        case USER_SIZE_CHANGED: {
            lv_obj_set_size(self->obj, ui_display_width, ui_display_height);
            break;
        }
    }
    return false;
}

void on_pc_added(const pcmanager_resp_t *resp, void *userdata) {
    launcher_controller_t *controller = userdata;
    if (!resp->server) return;
    PSERVER_LIST cur = NULL;
    for (cur = pcmanager_servers(pcmanager); cur != NULL; cur = cur->next) {
        const SERVER_DATA *server = cur->server;
        if (server == resp->server) {
            break;
        }
    }
    if (!cur) return;
    pclist_item_create(controller, cur);
}

void on_pc_updated(const pcmanager_resp_t *resp, void *userdata) {
    launcher_controller_t *controller = userdata;
    if (!resp->server) return;
    for (uint16_t i = 0, j = lv_obj_get_child_cnt(controller->pclist); i < j; i++) {
        lv_obj_t *child = lv_obj_get_child(controller->pclist, i);
        SERVER_LIST *cur = lv_obj_get_user_data(child);
        if (resp->server == cur->server) {
            const char *icon = server_item_icon(cur);
            lv_btn_set_icon(child, icon);
            break;
        }
    }
}

void on_pc_removed(const pcmanager_resp_t *resp, void *userdata) {
    launcher_controller_t *controller = userdata;
    if (!resp->server) return;
    for (uint16_t i = 0, j = lv_obj_get_child_cnt(controller->pclist); i < j; i++) {
        lv_obj_t *child = lv_obj_get_child(controller->pclist, i);
        SERVER_LIST *cur = lv_obj_get_user_data(child);
        if (resp->server == cur->server) {
            lv_obj_del(child);
            break;
        }
    }
}

static void cb_pc_selected(lv_event_t *event) {
    lv_obj_t *target = lv_event_get_target(event);
    if (lv_obj_get_parent(target) != lv_event_get_current_target(event)) return;
    launcher_controller_t *controller = lv_event_get_user_data(event);
    if (lv_fragment_manager_get_top(app_uimanager) != (void *) controller) return;
    PSERVER_LIST selected = lv_obj_get_user_data(target);
    launcher_select_server(controller, selected->server->uuid);
}

static void cb_pc_longpress(lv_event_t *event) {
    lv_obj_t *target = lv_event_get_target(event);
    if (lv_obj_get_parent(target) != lv_event_get_current_target(event)) return;
    PSERVER_LIST selected = lv_obj_get_user_data(target);
    lv_fragment_t *fragment = lv_fragment_create(&server_menu_class, selected);
    lv_obj_t *msgbox = lv_fragment_create_obj(fragment, NULL);
    lv_obj_add_event_cb(msgbox, ui_cb_destroy_fragment, LV_EVENT_DELETE, fragment);
}

static void select_pc(launcher_controller_t *controller, PSERVER_LIST selected, bool refocus) {
    if (selected) {
        lv_fragment_t *fragment = lv_fragment_create(&apps_controller_class, selected);
        lv_fragment_manager_replace(controller->base.child_manager, fragment, &controller->detail);
    } else {
        lv_fragment_manager_pop(controller->base.child_manager);
    }
    for (int i = 0, pclen = (int) lv_obj_get_child_cnt(controller->pclist); i < pclen; i++) {
        lv_obj_t *pcitem = lv_obj_get_child(controller->pclist, i);
        PSERVER_LIST cur = (PSERVER_LIST) lv_obj_get_user_data(pcitem);
        cur->selected = cur == selected;
        pcitem_set_selected(pcitem, cur->selected);
        if (refocus && cur->selected) {
            lv_group_focus_obj(pcitem);
        }
    }
}

static void update_pclist(launcher_controller_t *controller) {
    lv_obj_clean(controller->pclist);
    for (PSERVER_LIST cur = pcmanager_servers(pcmanager); cur != NULL; cur = cur->next) {
        lv_obj_t *pcitem = pclist_item_create(controller, cur);
        pcitem_set_selected(pcitem, cur->selected);
    }
}

static void pcitem_set_selected(lv_obj_t *pcitem, bool selected) {
    lv_obj_t *icon = lv_btn_find_img(pcitem);
    if (selected) {
        lv_obj_add_state(pcitem, LV_STATE_CHECKED);
        lv_obj_add_state(icon, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(pcitem, LV_STATE_CHECKED);
        lv_obj_clear_state(icon, LV_STATE_CHECKED);
    }
}

static lv_obj_t *pclist_item_create(launcher_controller_t *controller, PSERVER_LIST cur) {
    const SERVER_DATA *server = cur->server;
    const char *icon = server_item_icon(cur);
    lv_obj_t *pcitem = lv_list_add_btn(controller->pclist, icon, server->hostname);
    lv_obj_add_flag(pcitem, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_style(pcitem, &controller->nav_host_style, 0);
    lv_obj_t *btn_img = lv_btn_find_img(pcitem);
    lv_obj_set_style_text_font(btn_img, app_iconfonts.normal, 0);
    lv_obj_set_style_bg_opa(btn_img, LV_OPA_COVER, LV_STATE_CHECKED);
    lv_obj_set_style_text_color(btn_img, lv_color_black(), LV_STATE_CHECKED);
    lv_obj_set_style_radius(btn_img, LV_DPX(1), LV_STATE_CHECKED);
    lv_obj_set_style_outline_color(btn_img, lv_color_white(), LV_STATE_CHECKED);
    lv_obj_set_style_outline_opa(btn_img, LV_OPA_COVER, LV_STATE_CHECKED);
    lv_obj_set_style_outline_width(btn_img, LV_DPX(2), LV_STATE_CHECKED);
    lv_obj_set_user_data(pcitem, cur);
    return pcitem;
}

static const char *server_item_icon(const SERVER_LIST *node) {
    switch (node->state.code) {
        case SERVER_STATE_NONE:
        case SERVER_STATE_QUERYING:
            return MAT_SYMBOL_TV;
        case SERVER_STATE_AVAILABLE:
            return node->server->currentGame ? MAT_SYMBOL_ONDEMAND_VIDEO : MAT_SYMBOL_TV;
        case SERVER_STATE_NOT_PAIRED:
            return MAT_SYMBOL_LOCK;
        case SERVER_STATE_ERROR:
        case SERVER_STATE_OFFLINE:
            return MAT_SYMBOL_WARNING;
        default:
            return MAT_SYMBOL_TV;
    }
}

static void cb_detail_focused(lv_event_t *event) {
    launcher_controller_t *fragment = lv_event_get_user_data(event);
    if (!fragment->pane_initialized || fragment->detail_changing) return;
    lv_fragment_t *detail_fragment = lv_fragment_manager_find_by_container(fragment->base.child_manager,
                                                                           fragment->detail);
    if (!detail_fragment || lv_obj_get_parent(event->target) != detail_fragment->obj) return;
    set_detail_opened(fragment, true);
}

static void cb_detail_cancel(lv_event_t *event) {
    launcher_controller_t *controller = lv_event_get_user_data(event);
    set_detail_opened(controller, false);
}

static void cb_nav_focused(lv_event_t *event) {
    launcher_controller_t *controller = lv_event_get_user_data(event);
    if (!controller->pane_initialized) return;
    lv_obj_t *target = event->target;
    while (target && target != controller->nav) {
        target = lv_obj_get_parent(target);
    }
    if (!target) return;
    set_detail_opened(controller, false);
}

static void cb_nav_key(lv_event_t *event) {
    launcher_controller_t *fragment = lv_event_get_user_data(event);
    switch (lv_event_get_key(event)) {
        case LV_KEY_UP: {
            lv_group_t *group = fragment->nav_group;
            lv_group_focus_prev(group);
            break;
        }
        case LV_KEY_DOWN: {
            lv_group_t *group = fragment->nav_group;
            lv_group_focus_next(group);
            break;
        }
        case LV_KEY_RIGHT: {
            lv_fragment_t *detail_fragment = lv_fragment_manager_find_by_container(fragment->base.child_manager,
                                                                                   fragment->detail);
            if (detail_fragment) {
                set_detail_opened(fragment, true);
            }
            break;
        }
    }
}

static void set_detail_opened(launcher_controller_t *controller, bool opened) {
    bool key = ui_input_mode & UI_INPUT_MODE_BUTTON_FLAG;
    if (opened) {
        lv_obj_add_state(controller->detail, LV_STATE_USER_1);
        app_input_set_group(controller->detail_group);
        lv_obj_t *detail_focused = lv_group_get_focused(controller->detail_group);
        if (key && detail_focused) {
            if (lv_obj_check_type(detail_focused, &lv_gridview_class)) {
                int index = lv_gridview_get_focused_index(detail_focused);
                lv_gridview_focus(detail_focused, index);
            } else {
                lv_obj_add_state(detail_focused, LV_STATE_FOCUS_KEY);
            }
        }
    } else {
        lv_obj_clear_state(controller->detail, LV_STATE_USER_1);
        app_input_set_group(controller->nav_group);
        if (key) {
            lv_obj_t *nav_focused = lv_group_get_focused(controller->nav_group);
            if (nav_focused) {
                lv_obj_add_state(nav_focused, LV_STATE_FOCUS_KEY);
            }
        }
    }
    controller->detail_opened = opened;
}

static void open_manual_add(lv_event_t *event) {
    launcher_controller_t *controller = lv_event_get_user_data(event);
    lv_fragment_t *fragment = lv_fragment_create(&add_dialog_class, NULL);
    lv_obj_t *msgbox = lv_fragment_create_obj(fragment, NULL);
    lv_obj_add_event_cb(msgbox, ui_cb_destroy_fragment, LV_EVENT_DELETE, fragment);
}

static void open_settings(lv_event_t *event) {
    launcher_controller_t *launcher = event->user_data;
    lv_fragment_t *fragment = lv_fragment_create(&settings_controller_cls, NULL);
    lv_obj_t *const *container = lv_fragment_get_container(lv_fragment_manager_get_top(app_uimanager));
    lv_fragment_manager_push(app_uimanager, fragment, container);
}

static void show_decoder_error(launcher_controller_t *controller) {
    static const char *btn_txts[] = {translatable("OK"), ""};
    lv_obj_t *msgbox = lv_msgbox_create_i18n(NULL, locstr("Decoder not working"), "placeholder", btn_txts, false);
    lv_obj_add_event_cb(msgbox, decoder_error_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_t *msgview = lv_msgbox_get_text(msgbox);
    const char *module_err = module_geterror();
    if (module_err[0]) {
        lv_label_set_text_fmt(msgview, locstr("Unable to initialize decoder %s. Please try other decoders.\n"
                                              "Error detail: %s"),
                              decoder_pref_requested >= DECODER_FIRST ? decoder_definitions[decoder_pref_requested].name
                                                                      : app_configuration->decoder, module_err);
    } else {
        lv_label_set_text_fmt(msgview, locstr("Unable to initialize decoder %s. Please try other decoders."),
                              decoder_pref_requested >= DECODER_FIRST ? decoder_definitions[decoder_pref_requested].name
                                                                      : app_configuration->decoder);
    }
    lv_obj_center(msgbox);
}

static void decoder_error_cb(lv_event_t *e) {
    lv_obj_t *msgbox = lv_event_get_current_target(e);
    lv_msgbox_close_async(msgbox);
}

static void open_help(lv_event_t *event) {
    help_dialog_create();
}