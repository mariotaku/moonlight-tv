#include "app.h"
#include "app_launch.h"

#include "add.dialog.h"
#include "apps.controller.h"
#include "launcher.controller.h"
#include "pair.dialog.h"
#include "server.context_menu.h"

#include "ui/help/help.dialog.h"
#include "ui/root.h"
#include "ui/settings/settings.controller.h"

#include "lvgl/font/material_icons_regular_symbols.h"
#include "lvgl/util/lv_app_utils.h"
#include "lv_gridview.h"

#include "util/font.h"
#include "util/i18n.h"
#include "util/user_event.h"
#include "logging.h"

static void launcher_controller(lv_fragment_t *self, void *args);

static void controller_dtor(lv_fragment_t *self);

static void launcher_view_init(lv_fragment_t *self, lv_obj_t *view);

static void launcher_view_destroy(lv_fragment_t *self, lv_obj_t *view);

static bool launcher_event_cb(lv_fragment_t *self, int code, void *userdata);

static void on_pc_added(const uuidstr_t *uuid, void *userdata);

static void on_pc_updated(const uuidstr_t *uuid, void *userdata);

static void on_pc_removed(const uuidstr_t *uuid, void *userdata);

static void update_pclist(launcher_fragment_t *controller);

static void cb_pc_selected(lv_event_t *event);

static void cb_pc_longpress(lv_event_t *event);

static void cb_nav_focused(lv_event_t *event);

static void cb_nav_key(lv_event_t *event);

static void cb_detail_focused(lv_event_t *event);

static void cb_detail_cancel(lv_event_t *event);

static void open_manual_add(lv_event_t *event);

static void open_settings(lv_event_t *event);

static void open_help(lv_event_t *event);

static void select_pc(launcher_fragment_t *controller, const uuidstr_t *uuid, bool refocus);

static void set_detail_opened(launcher_fragment_t *controller, bool opened);

static lv_obj_t *pclist_item_create(launcher_fragment_t *fragment, const pclist_t *node);

static void pclist_item_deleted(lv_event_t *e);

static const char *server_item_icon(const pclist_t *node);

static void pcitem_set_selected(lv_obj_t *pcitem, bool selected);

static void show_decoder_error();

static void show_conf_persistent_error();

static void decoder_error_cb(lv_event_t *e);

static void populate_selected_host(launcher_fragment_t *controller);

const lv_fragment_class_t launcher_controller_class = {
        .constructor_cb = launcher_controller,
        .destructor_cb = controller_dtor,
        .create_obj_cb = launcher_win_create,
        .obj_created_cb = launcher_view_init,
        .obj_will_delete_cb = launcher_view_destroy,
        .event_cb = launcher_event_cb,
        .instance_size = sizeof(launcher_fragment_t),
};

static const pcmanager_listener_t pcmanager_callbacks = {
        .added = on_pc_added,
        .updated = on_pc_updated,
        .removed = on_pc_removed,
};

static launcher_fragment_t *current_instance = NULL;

launcher_fragment_t *launcher_instance() {
    return current_instance;
}

void launcher_select_server(launcher_fragment_t *controller, const uuidstr_t *uuid) {
    const pclist_t *node = uuid ? pcmanager_node(pcmanager, uuid) : NULL;
    if (!node) {
        set_detail_opened(controller, false);
        select_pc(controller, NULL, false);
        return;
    }
    if (node->state.code == SERVER_STATE_NOT_PAIRED) {
        pair_dialog_open(uuid);
        return;
    }
    set_detail_opened(controller, true);
    if (node->selected) { return; }
    select_pc(controller, uuid, false);
}

static void launcher_controller(lv_fragment_t *self, void *args) {
    launcher_fragment_t *fragment = (launcher_fragment_t *) self;
    launcher_fragment_args_t *fargs = args;
    fragment->global = fargs->app;
    app_ui_t *ui = &fragment->global->ui;
    static const lv_style_prop_t props[] = {
            LV_STYLE_OPA, LV_STYLE_BG_OPA, LV_STYLE_TRANSLATE_X, LV_STYLE_TRANSLATE_Y, 0
    };
    lv_style_transition_dsc_init(&fragment->tr_detail, props, lv_anim_path_ease_out, 300, 0, NULL);
    lv_style_transition_dsc_init(&fragment->tr_nav, props, lv_anim_path_ease_out, 350, 0, NULL);

    lv_style_init(&fragment->nav_host_style);
    int ico_width_def = lv_txt_get_width(MAT_SYMBOL_SETTINGS, sizeof(MAT_SYMBOL_SETTINGS), ui->fonts.icons.normal, 0,
                                         0);
    int host_icon_pad = (LV_DPX(NAV_WIDTH_COLLAPSED) - ico_width_def) / 2;
    lv_style_set_pad_left(&fragment->nav_host_style, host_icon_pad);
    lv_style_set_pad_gap(&fragment->nav_host_style, host_icon_pad);

    lv_style_init(&fragment->nav_menu_style);
    lv_style_set_border_side(&fragment->nav_menu_style, LV_BORDER_SIDE_NONE);
    lv_style_set_pad_ver(&fragment->nav_menu_style, LV_DPX(10));
    int ico_width_sm = lv_txt_get_width(MAT_SYMBOL_SETTINGS, sizeof(MAT_SYMBOL_SETTINGS), ui->fonts.icons.small, 0, 0);
    int nav_icon_pad = (LV_DPX(NAV_WIDTH_COLLAPSED) - ico_width_sm) / 2;
    lv_style_set_pad_left(&fragment->nav_menu_style, nav_icon_pad);
    lv_style_set_pad_gap(&fragment->nav_menu_style, nav_icon_pad);

    fragment->detail_opened = false;
    fragment->pane_initialized = false;
    fragment->first_created = true;
    fragment->launch_params = fargs->params;
}

static void controller_dtor(lv_fragment_t *self) {
    launcher_fragment_t *controller = (launcher_fragment_t *) self;
    lv_style_reset(&controller->nav_menu_style);
    lv_style_reset(&controller->nav_host_style);
}

static void launcher_view_init(lv_fragment_t *self, lv_obj_t *view) {
    LV_UNUSED(view);
    launcher_fragment_t *fragment = (launcher_fragment_t *) self;
    pcmanager_register_listener(pcmanager, &pcmanager_callbacks, fragment);
    lv_obj_add_event_cb(fragment->nav, cb_nav_focused, LV_EVENT_FOCUSED, fragment);
    lv_obj_add_event_cb(fragment->nav, cb_nav_key, LV_EVENT_KEY, fragment);
    lv_obj_add_event_cb(fragment->nav, app_quit_confirm, LV_EVENT_CANCEL, fragment);
    lv_obj_add_event_cb(fragment->detail, cb_detail_focused, LV_EVENT_FOCUSED, fragment);
    lv_obj_add_event_cb(fragment->detail, cb_detail_cancel, LV_EVENT_CANCEL, fragment);
    lv_obj_add_event_cb(fragment->pclist, cb_pc_selected, LV_EVENT_SHORT_CLICKED, fragment);
    lv_obj_add_event_cb(fragment->pclist, cb_pc_longpress, LV_EVENT_LONG_PRESSED, fragment);
    lv_obj_add_event_cb(fragment->add_btn, open_manual_add, LV_EVENT_CLICKED, fragment);
    lv_obj_add_event_cb(fragment->pref_btn, open_settings, LV_EVENT_CLICKED, fragment);
    lv_obj_add_event_cb(fragment->help_btn, open_help, LV_EVENT_CLICKED, fragment);
    lv_obj_add_event_cb(fragment->quit_btn, app_quit_confirm, LV_EVENT_CLICKED, fragment);

    update_pclist(fragment);

    populate_selected_host(fragment);

    for (const pclist_t *cur = pcmanager_servers(pcmanager); cur != NULL; cur = cur->next) {
        if (cur->selected) {
            select_pc(fragment, &cur->id, true);
            if (fragment->first_created) {
                fragment->detail_opened = true;
            }
            continue;
        }
        pcmanager_request_update(pcmanager, &cur->id, NULL, NULL);
    }
    fragment->pane_initialized = true;
    set_detail_opened(fragment, fragment->detail_opened);
    pcmanager_auto_discovery_start(pcmanager);

    lv_obj_set_style_transition(fragment->detail, &fragment->tr_nav, 0);
    lv_obj_set_style_transition(fragment->detail, &fragment->tr_detail, LV_STATE_USER_1);
    current_instance = fragment;

    if (fragment->first_created) {
        if (fragment->global->ss4s.selection.video_module == NULL ||
            fragment->global->ss4s.selection.audio_module == NULL) {
            show_decoder_error();
        }
        if (!app_configuration->conf_persistent) {
            show_conf_persistent_error();
        }
    }
    fragment->first_created = false;
}

static void launcher_view_destroy(lv_fragment_t *self, lv_obj_t *view) {
    launcher_fragment_t *controller = (launcher_fragment_t *) self;
    LV_UNUSED(view);
    current_instance = NULL;
    app_input_set_group(&controller->global->ui.input, NULL);
    pcmanager_auto_discovery_stop(pcmanager);

    controller->pane_initialized = false;
    controller->launch_params = NULL;

    lv_group_del(controller->nav_group);
    lv_group_del(controller->detail_group);

    pcmanager_unregister_listener(pcmanager, &pcmanager_callbacks);
}

static bool launcher_event_cb(lv_fragment_t *self, int code, void *userdata) {
    LV_UNUSED(userdata);
    launcher_fragment_t *fragment = (launcher_fragment_t *) self;
    if (code == USER_SIZE_CHANGED) {
        lv_obj_set_size(self->obj, fragment->global->ui.width, fragment->global->ui.height);
    }
    return false;
}

void on_pc_added(const uuidstr_t *uuid, void *userdata) {
    launcher_fragment_t *controller = userdata;
    const pclist_t *node = pcmanager_node(pcmanager, uuid);
    if (node == NULL) { return; }
    pclist_item_create(controller, node);

    populate_selected_host(controller);
}

void on_pc_updated(const uuidstr_t *uuid, void *userdata) {
    launcher_fragment_t *controller = userdata;
    for (uint16_t i = 0, j = lv_obj_get_child_cnt(controller->pclist); i < j; i++) {
        lv_obj_t *child = lv_obj_get_child(controller->pclist, i);
        const uuidstr_t *item_id = (const uuidstr_t *) lv_obj_get_user_data(child);
        if (!uuidstr_t_equals_t(uuid, item_id)) { continue; }
        const pclist_t *node = pcmanager_node(pcmanager, item_id);
        const char *icon = server_item_icon(node);
        lv_btn_set_icon(child, icon);
        break;
    }
}

void on_pc_removed(const uuidstr_t *uuid, void *userdata) {
    launcher_fragment_t *controller = userdata;
    for (uint16_t i = 0, j = lv_obj_get_child_cnt(controller->pclist); i < j; i++) {
        lv_obj_t *child = lv_obj_get_child(controller->pclist, i);
        const uuidstr_t *item_id = (const uuidstr_t *) lv_obj_get_user_data(child);
        if (!uuidstr_t_equals_t(uuid, item_id)) { continue; }
        lv_obj_del(child);
        break;
    }
}

static void cb_pc_selected(lv_event_t *event) {
    lv_obj_t *target = lv_event_get_target(event);
    if (lv_obj_get_parent(target) != lv_event_get_current_target(event)) { return; }
    launcher_fragment_t *controller = lv_event_get_user_data(event);
    if (lv_fragment_manager_get_top(controller->global->ui.fm) != (void *) controller) { return; }
    const uuidstr_t *uuid = (const uuidstr_t *) lv_obj_get_user_data(target);
    launcher_select_server(controller, uuid);
}

static void cb_pc_longpress(lv_event_t *event) {
    lv_obj_t *target = lv_event_get_target(event);
    if (lv_obj_get_parent(target) != lv_event_get_current_target(event)) { return; }
    const uuidstr_t *uuid = (const uuidstr_t *) lv_obj_get_user_data(target);
    lv_fragment_t *fragment = lv_fragment_create(&server_menu_class, (void *) uuid);
    lv_obj_t *msgbox = lv_fragment_create_obj(fragment, NULL);
    lv_obj_add_event_cb(msgbox, ui_cb_destroy_fragment, LV_EVENT_DELETE, fragment);
}

static void select_pc(launcher_fragment_t *controller, const uuidstr_t *uuid, bool refocus) {
    if (uuid) {
        apps_fragment_arg_t arg = {.global = controller->global, .host = *uuid};
        const app_launch_params_t *params = controller->launch_params;
        if (!controller->def_app_requested && params != NULL && uuidstr_t_equals_t(uuid, &params->default_host_uuid)) {
            controller->def_app_requested = true;
            arg.def_app = params->default_app_id;
        }
        lv_fragment_t *fragment = lv_fragment_create(&apps_controller_class, &arg);
        lv_fragment_manager_replace(controller->base.child_manager, fragment, &controller->detail);
    } else {
        lv_fragment_manager_pop(controller->base.child_manager);
    }
    for (int i = 0, pclen = (int) lv_obj_get_child_cnt(controller->pclist); i < pclen; i++) {
        lv_obj_t *pcitem = lv_obj_get_child(controller->pclist, i);
        const uuidstr_t *cur_id = (const uuidstr_t *) lv_obj_get_user_data(pcitem);
        if (uuidstr_t_equals_t(cur_id, uuid)) {
            pcmanager_select(pcmanager, uuid);
            pcitem_set_selected(pcitem, true);
            if (refocus) {
                lv_group_focus_obj(pcitem);
            }
        } else {
            pcitem_set_selected(pcitem, false);
        }
    }
}

static void update_pclist(launcher_fragment_t *controller) {
    lv_obj_clean(controller->pclist);
    for (const pclist_t *cur = pcmanager_servers(pcmanager); cur != NULL; cur = cur->next) {
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

static lv_obj_t *pclist_item_create(launcher_fragment_t *fragment, const pclist_t *node) {
    app_ui_t *ui = &fragment->global->ui;

    const SERVER_DATA *server = node->server;
    const char *icon = server_item_icon(node);
    lv_obj_t *pcitem = lv_list_add_btn(fragment->pclist, icon, server->hostname);
    lv_obj_add_flag(pcitem, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_style(pcitem, &fragment->nav_host_style, 0);
    lv_obj_t *btn_img = lv_btn_find_img(pcitem);
    lv_obj_set_style_text_font(btn_img, ui->fonts.icons.normal, 0);
    lv_obj_set_style_bg_opa(btn_img, LV_OPA_COVER, LV_STATE_CHECKED);
    lv_obj_set_style_text_color(btn_img, lv_color_black(), LV_STATE_CHECKED);
    lv_obj_set_style_radius(btn_img, LV_DPX(1), LV_STATE_CHECKED);
    lv_obj_set_style_outline_color(btn_img, lv_color_white(), LV_STATE_CHECKED);
    lv_obj_set_style_outline_opa(btn_img, LV_OPA_COVER, LV_STATE_CHECKED);
    lv_obj_set_style_outline_width(btn_img, LV_DPX(2), LV_STATE_CHECKED);
    uuidstr_t *uuid = SDL_malloc(sizeof(uuidstr_t));
    *uuid = node->id;
    lv_obj_set_user_data(pcitem, uuid);
    lv_obj_add_event_cb(pcitem, pclist_item_deleted, LV_EVENT_DELETE, NULL);
    return pcitem;
}

static void pclist_item_deleted(lv_event_t *e) {
    lv_obj_t *target = lv_event_get_target(e);
    void *uuid = lv_obj_get_user_data(target);
    SDL_free(uuid);
}

static const char *server_item_icon(const pclist_t *node) {
    if (node == NULL) {
        return MAT_SYMBOL_WARNING;
    }
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
    launcher_fragment_t *fragment = lv_event_get_user_data(event);
    if (!fragment->pane_initialized || fragment->detail_changing) { return; }
    lv_fragment_t *detail_fragment = lv_fragment_manager_find_by_container(fragment->base.child_manager,
                                                                           fragment->detail);
    if (!detail_fragment || lv_obj_get_parent(event->target) != detail_fragment->obj) { return; }
    set_detail_opened(fragment, true);
}

static void cb_detail_cancel(lv_event_t *event) {
    launcher_fragment_t *controller = lv_event_get_user_data(event);
    set_detail_opened(controller, false);
}

static void cb_nav_focused(lv_event_t *event) {
    launcher_fragment_t *controller = lv_event_get_user_data(event);
    if (!controller->pane_initialized) { return; }
    lv_obj_t *target = event->target;
    while (target && target != controller->nav) {
        target = lv_obj_get_parent(target);
    }
    if (!target) { return; }
    set_detail_opened(controller, false);
}

static void cb_nav_key(lv_event_t *event) {
    launcher_fragment_t *fragment = lv_event_get_user_data(event);
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

static void set_detail_opened(launcher_fragment_t *controller, bool opened) {
    bool key = app_ui_get_input_mode(&controller->global->ui.input) & UI_INPUT_MODE_BUTTON_FLAG;
    if (opened) {
        lv_obj_add_state(controller->detail, LV_STATE_USER_1);
        app_input_set_group(&controller->global->ui.input, controller->detail_group);
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
        app_input_set_group(&controller->global->ui.input, controller->nav_group);
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
    LV_UNUSED(event);
    lv_fragment_t *fragment = lv_fragment_create(&add_dialog_class, NULL);
    lv_obj_t *msgbox = lv_fragment_create_obj(fragment, NULL);
    lv_obj_add_event_cb(msgbox, ui_cb_destroy_fragment, LV_EVENT_DELETE, fragment);
}

static void open_settings(lv_event_t *event) {
    launcher_fragment_t *self = lv_event_get_user_data(event);
    lv_fragment_t *fragment = lv_fragment_create(&settings_controller_cls, self->global);
    lv_obj_t *const *container = lv_fragment_get_container(lv_fragment_manager_get_top(self->global->ui.fm));
    lv_fragment_manager_push(self->global->ui.fm, fragment, container);
}

static void show_decoder_error() {
    static const char *btn_txts[] = {translatable("OK"), ""};
    lv_obj_t *msgbox = lv_msgbox_create_i18n(NULL, locstr("No working decoder"), "placeholder", btn_txts, false);
    lv_obj_add_event_cb(msgbox, decoder_error_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_t *msgview = lv_msgbox_get_text(msgbox);
    lv_label_set_text_static(msgview, locstr("Streaming can't work without a valid decoder."));
    lv_obj_center(msgbox);
}

static void show_conf_persistent_error() {
    static const char *btn_txts[] = {translatable("OK"), ""};
    lv_obj_t *msgbox = lv_msgbox_create_i18n(NULL, locstr("Can't save settings"), "placeholder", btn_txts, false);
    lv_obj_add_event_cb(msgbox, decoder_error_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_t *msgview = lv_msgbox_get_text(msgbox);
    lv_label_set_text_static(msgview, locstr("Can't find a writable directory to save settings. Settings and pairing "
                                             "information will be lost when the TV is turned off.\n\n"
                                             "(If you're using webOS 3.0 or newer, restart the TV may fix this issue.)"));
    lv_obj_center(msgbox);
}

static void decoder_error_cb(lv_event_t *e) {
    lv_obj_t *msgbox = lv_event_get_current_target(e);
    lv_msgbox_close_async(msgbox);
}

static void open_help(lv_event_t *event) {
    LV_UNUSED(event);
    help_dialog_create();
}

static void populate_selected_host(launcher_fragment_t *controller) {
    // Easy and dirty way to select preferred host
    if (controller->def_host_selected || controller->launch_params == NULL ||
        uuidstr_is_empty(&controller->launch_params->default_host_uuid)) {
        return;
    }
    for (const pclist_t *cur = pcmanager_servers(pcmanager); cur != NULL; cur = cur->next) {
        if (uuidstr_t_equals_t(&cur->id, &controller->launch_params->default_host_uuid)) {
            commons_log_info("UI", "Host %s was selected", cur->server->hostname);
            pcmanager_select(pcmanager, &cur->id);
            controller->def_host_selected = true;
            break;
        }
    }
}