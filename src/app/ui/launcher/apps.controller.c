//
// Created by Mariotaku on 2021/08/31.
//

#include "app.h"
#include "appitem.view.h"
#include "apps.controller.h"
#include "launcher.controller.h"
#include "ui/streaming/streaming.controller.h"

#include "coverloader.h"
#include "backend/apploader/apploader.h"
#include <errors.h>
#include <assert.h>

#include "lvgl/lv_ext_utils.h"
#include "lvgl/util/lv_app_utils.h"
#include "lv_gridview.h"

#include "util/user_event.h"
#include "util/i18n.h"
#include "pair.dialog.h"
#include "ui/common/progress_dialog.h"

typedef void (*action_cb_t)(apps_fragment_t *controller, lv_obj_t *buttons, uint16_t index);

static lv_obj_t *apps_view(lv_fragment_t *self, lv_obj_t *container);

static void on_view_created(lv_fragment_t *self, lv_obj_t *view);

static void obj_will_delete(lv_fragment_t *self, lv_obj_t *obj);

static void on_destroy_view(lv_fragment_t *self, lv_obj_t *view);

static bool on_event(lv_fragment_t *self, int code, void *userdata);

static void host_info_cb(int result, const char *error, const uuidstr_t *uuid, void *userdata);

static void send_wol_cb(int result, const char *error, const uuidstr_t *uuid, void *userdata);

static void on_host_updated(const uuidstr_t *uuid, void *userdata);

static void on_host_removed(const uuidstr_t *uuid, void *userdata);

static void item_click_cb(lv_event_t *event);

static void item_longpress_cb(lv_event_t *event);

static void launcher_launch_game(apps_fragment_t *controller, const apploader_item_t *app);

static void launcher_toggle_fav(apps_fragment_t *controller, const apploader_item_t *app);

static void launcher_toggle_hidden(apps_fragment_t *controller, const apploader_item_t *app);

static void launcher_quit_game(apps_fragment_t *controller);

static void applist_focus_enter(lv_event_t *event);

static void applist_focus_leave(lv_event_t *event);

static void update_view_state(apps_fragment_t *controller);

static void appitem_bind(apps_fragment_t *controller, lv_obj_t *item, apploader_item_t *app);

static int adapter_item_count(lv_obj_t *, void *data);

static lv_obj_t *adapter_create_view(lv_obj_t *parent);

static void adapter_bind_view(lv_obj_t *, lv_obj_t *, void *data, int position);

static void quitgame_cb(int result, const char *error, const uuidstr_t *uuid, void *userdata);

static void apps_controller_ctor(lv_fragment_t *self, void *args);

static void apps_controller_dtor(lv_fragment_t *self);

static void appload_started(void *userdata);

static void appload_loaded(apploader_list_t *apps, void *userdata);

static void appload_errored(int code, const char *error, void *userdata);

static void quit_dialog_cb(lv_event_t *event);

static void actions_click_cb(lv_event_t *event);

static void action_cb_wol(apps_fragment_t *controller, lv_obj_t *buttons, uint16_t index);

static void action_cb_host_reload(apps_fragment_t *controller, lv_obj_t *buttons, uint16_t index);

static void action_cb_pair(apps_fragment_t *controller, lv_obj_t *buttons, uint16_t index);

static void update_grid_config(apps_fragment_t *controller);

static void open_context_menu(apps_fragment_t *fragment, appitem_viewholder_t *holder);

static void context_menu_cancel_cb(lv_event_t *event);

static void context_menu_click_cb(lv_event_t *event);

static void app_detail_dialog(apps_fragment_t *fragment, const apploader_item_t *app);

static void app_detail_click_cb(lv_event_t *event);

static void set_actions(apps_fragment_t *controller, const char **labels, const action_cb_t *callbacks);

/**
 *
 * @param old_list
 * @param new_list
 * @param num_changes number of changes. It will be assigned to -1 if the whole dataset has been changed.
 * @return Allocated array of changes. It should be freed by caller.
 */
static lv_gridview_data_change_t *apps_list_detect_change(const apploader_list_t *old_list,
                                                          const apploader_list_t *new_list, int *num_changes);

static void show_progress(apps_fragment_t *fragment);

static void show_ok(apps_fragment_t *fragment);

static void show_error(apps_fragment_t *fragment, const char *title, const char *hint, const char *detail);

static apps_fragment_t *current_instance = NULL;

const static lv_gridview_adapter_t apps_adapter = {
        .item_count = adapter_item_count,
        .create_view = adapter_create_view,
        .bind_view = adapter_bind_view,
};
const static pcmanager_listener_t pc_listeners = {
        .updated = on_host_updated,
        .removed = on_host_removed,
};

const lv_fragment_class_t apps_controller_class = {
        .constructor_cb = apps_controller_ctor,
        .destructor_cb = apps_controller_dtor,
        .create_obj_cb = apps_view,
        .obj_will_delete_cb = obj_will_delete,
        .obj_created_cb = on_view_created,
        .obj_deleted_cb = on_destroy_view,
        .event_cb = on_event,
        .instance_size = sizeof(apps_fragment_t),
};

static const char *action_labels_offline[] = {translatable("Wake"), translatable("Retry"), ""};
static const action_cb_t action_callbacks_offline[] = {action_cb_wol, action_cb_host_reload};
static const char *action_labels_error[] = {translatable("Retry"), ""};
static const action_cb_t action_callbacks_error[] = {action_cb_host_reload};
static const char *actions_unpaired[] = {translatable("Pair"), ""};
static const action_cb_t action_callbacks_unpaired[] = {action_cb_pair};
static const char *actions_apps_none[] = {""};

static void apps_controller_ctor(lv_fragment_t *self, void *args) {
    apps_fragment_t *controller = (apps_fragment_t *) self;
    controller->apploader_cb.start = appload_started;
    controller->apploader_cb.data = appload_loaded;
    controller->apploader_cb.error = appload_errored;
    apps_fragment_arg_t *arg = args;
    controller->global = arg->global;
    controller->uuid = arg->host;
    controller->def_app = arg->def_app;
    controller->node = pcmanager_node(pcmanager, &controller->uuid);
    controller->apploader = apploader_create(arg->global, &controller->uuid, &controller->apploader_cb, controller);

    appitem_style_init(&controller->appitem_style);
}

static void apps_controller_dtor(lv_fragment_t *self) {
    apps_fragment_t *fragment = (apps_fragment_t *) self;
    appitem_style_deinit(&fragment->appitem_style);
    apploader_destroy(fragment->apploader);
    if (fragment->apploader_apps != NULL) {
        apploader_list_free(fragment->apploader_apps);
    }
}

static lv_obj_t *apps_view(lv_fragment_t *self, lv_obj_t *container) {
    apps_fragment_t *controller = (apps_fragment_t *) self;
    lv_obj_t *view = lv_obj_create(container);
    lv_obj_remove_style_all(view);
    lv_obj_add_flag(view, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_size(view, LV_PCT(100), LV_PCT(100));
    lv_obj_set_scroll_dir(view, LV_DIR_NONE);

    lv_obj_t *applist = controller->applist = lv_gridview_create(view);
    lv_obj_add_flag(applist, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_scroll_dir(applist, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(applist, LV_SCROLLBAR_MODE_ACTIVE);
    lv_obj_set_style_pad_all(applist, lv_dpx(24), 0);
    lv_obj_set_style_pad_gap(applist, lv_dpx(24), 0);
    lv_obj_set_style_radius(applist, 0, 0);
    lv_obj_set_style_border_side(applist, LV_BORDER_SIDE_NONE, 0);
    lv_obj_set_style_bg_opa(applist, 0, 0);
    lv_obj_set_size(applist, LV_PCT(100), LV_PCT(100));
    lv_obj_update_layout(applist);

    lv_gridview_set_adapter(applist, &apps_adapter);
    lv_obj_t *appload = controller->appload = lv_spinner_create(view, 1000, 60);
    launcher_fragment_t *parent_controller = (launcher_fragment_t *) lv_fragment_get_parent(&controller->base);
    lv_group_add_obj(parent_controller->detail_group, appload);
    lv_obj_add_flag(appload, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_size(appload, lv_dpx(60), lv_dpx(60));
    lv_obj_center(appload);

    lv_obj_t *apperror = controller->apperror = lv_obj_create(view);
    lv_obj_add_flag(apperror, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_size(apperror, LV_PCT(80), LV_PCT(60));
    lv_obj_center(apperror);
    lv_obj_set_flex_flow(apperror, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(apperror, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER);
    lv_obj_t *errortitle = controller->errortitle = lv_label_create(apperror);
    lv_obj_set_width(errortitle, LV_PCT(100));
    lv_obj_set_style_text_font(errortitle, lv_theme_get_font_large(apperror), 0);
    lv_obj_t *errorlabel = controller->errorhint = lv_label_create(apperror);
    lv_obj_set_width(errorlabel, LV_PCT(100));
    lv_obj_t *errordetail = controller->errordetail = lv_label_create(apperror);
    lv_obj_set_style_border_width(errordetail, LV_DPX(2), 0);
    lv_obj_set_style_border_opa(errordetail, LV_OPA_50, 0);
    lv_obj_set_style_border_color(errordetail, lv_palette_main(LV_PALETTE_BLUE_GREY), 0);
    lv_obj_set_style_pad_all(errordetail, LV_DPX(10), 0);
    lv_obj_set_style_radius(errordetail, LV_DPX(20), 0);
    lv_obj_set_width(errordetail, LV_PCT(100));
    lv_obj_set_flex_grow(errordetail, 1);

    lv_obj_t *actions = controller->actions = lv_btnmatrix_create(apperror);
    lv_obj_set_style_outline_width(actions, 0, 0);
    lv_obj_set_style_outline_width(actions, 0, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_pad_all(actions, LV_DPX(5), LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_side(actions, LV_BORDER_SIDE_NONE, 0);
    const lv_font_t *font = lv_obj_get_style_text_font(actions, LV_PART_ITEMS);
    lv_obj_set_height(actions, lv_font_get_line_height(font) + LV_DPI_DEF / 10 + LV_DPX(5) * 2);
    lv_obj_set_style_max_width(actions, LV_PCT(100), 0);
    lv_obj_add_flag(actions, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_btnmatrix_set_btn_ctrl_all(actions, LV_BTNMATRIX_CTRL_CLICK_TRIG | LV_BTNMATRIX_CTRL_NO_REPEAT);

    set_actions(controller, actions_apps_none, NULL);

    return view;
}

static void on_view_created(lv_fragment_t *self, lv_obj_t *view) {
    LV_UNUSED(view);
    apps_fragment_t *controller = (apps_fragment_t *) self;
    controller->coverloader = coverloader_new(controller->global);
    pcmanager_register_listener(pcmanager, &pc_listeners, controller);
    lv_obj_t *applist = controller->applist;
    lv_obj_add_event_cb(applist, item_click_cb, LV_EVENT_SHORT_CLICKED, controller);
    lv_obj_add_event_cb(applist, item_longpress_cb, LV_EVENT_LONG_PRESSED, controller);
    lv_obj_add_event_cb(applist, applist_focus_enter, LV_EVENT_FOCUSED, controller);
    lv_obj_add_event_cb(applist, applist_focus_leave, LV_EVENT_DEFOCUSED, controller);
    lv_obj_add_event_cb(applist, applist_focus_leave, LV_EVENT_LEAVE, controller);
    lv_obj_add_event_cb(controller->actions, actions_click_cb, LV_EVENT_VALUE_CHANGED, controller);

    update_grid_config(controller);
    lv_obj_set_user_data(controller->applist, controller);

    const SERVER_STATE *state = pcmanager_state(pcmanager, &controller->uuid);
    if (state->code != SERVER_STATE_QUERYING) {
        pcmanager_request_update(pcmanager, &controller->uuid, host_info_cb, controller);
        if (state->code == SERVER_STATE_AVAILABLE) {
            apploader_load(controller->apploader);
        }
    }
    current_instance = controller;
    update_view_state(controller);
}

static void obj_will_delete(lv_fragment_t *self, lv_obj_t *obj) {
    LV_UNUSED(obj);
    apps_fragment_t *fragment = (apps_fragment_t *) self;
    apploader_cancel(fragment->apploader);
    fragment->def_app = 0;
}

static void update_grid_config(apps_fragment_t *controller) {
    lv_obj_t *applist = controller->applist;
    lv_obj_update_layout(applist);
    lv_coord_t applist_width = lv_obj_get_width(applist);
    int col_count = LV_CLAMP(2, applist_width / lv_dpx(120), 5);
    lv_coord_t col_width = (applist_width - lv_obj_get_style_pad_left(applist, 0) -
                            lv_obj_get_style_pad_right(applist, 0) -
                            lv_obj_get_style_pad_column(applist, 0) * (col_count - 1)) / col_count;
    controller->col_count = col_count;
    controller->col_width = col_width;
    lv_coord_t row_height = col_width / 3 * 4;
    controller->col_height = row_height;
    lv_gridview_set_config(applist, col_count, row_height, LV_GRID_ALIGN_CENTER, LV_GRID_ALIGN_CENTER);

    controller->appitem_style.defcover_src.header.w = col_width;
    controller->appitem_style.defcover_src.header.h = row_height;
}

static void on_destroy_view(lv_fragment_t *self, lv_obj_t *view) {
    LV_UNUSED(view);
    current_instance = NULL;
    apps_fragment_t *controller = (apps_fragment_t *) self;
    controller->show_hidden_apps = false;
    pcmanager_unregister_listener(pcmanager, &pc_listeners);
    coverloader_unref(controller->coverloader);
}

static bool on_event(lv_fragment_t *self, int code, void *userdata) {
    LV_UNUSED(userdata);
    apps_fragment_t *controller = (apps_fragment_t *) self;
    switch (code) {
        case USER_SIZE_CHANGED: {
            update_grid_config(controller);
            lv_gridview_rebind(controller->applist);
            break;
        }
        case USER_SHOW_HIDDEN_APPS: {
            ui_userevent_t *event = userdata;
            if (uuidstr_t_equals_t(&controller->uuid, event->data1)) {
                controller->show_hidden_apps = true;
                apploader_load(controller->apploader);
            }
            free(event->data1);
            return true;
        }
        default:
            break;
    }
    return false;
}

static void on_host_updated(const uuidstr_t *uuid, void *userdata) {
    apps_fragment_t *controller = (apps_fragment_t *) userdata;
    if (controller != current_instance) { return; }
    if (!uuidstr_t_equals_t(&controller->uuid, uuid)) { return; }
    const SERVER_STATE *state = pcmanager_state(pcmanager, uuid);
    assert(state != NULL);
    if (state->code == SERVER_STATE_AVAILABLE) {
        apploader_load(controller->apploader);
    }
    update_view_state(controller);
}

static void on_host_removed(const uuidstr_t *uuid, void *userdata) {
    apps_fragment_t *controller = (apps_fragment_t *) userdata;
    if (!uuidstr_t_equals_t(&controller->uuid, uuid)) { return; }
    controller->node = NULL;
    lv_fragment_del((lv_fragment_t *) controller);
}

static void host_info_cb(int result, const char *error, const uuidstr_t *uuid, void *userdata) {
    apps_fragment_t *controller = (apps_fragment_t *) userdata;
    if (controller != current_instance) { return; }
    if (!controller->base.managed->obj_created) { return; }
    lv_btnmatrix_clear_btn_ctrl_all(controller->actions, LV_BTNMATRIX_CTRL_DISABLED);
}

static void send_wol_cb(int result, const char *error, const uuidstr_t *uuid, void *userdata) {
    apps_fragment_t *controller = (apps_fragment_t *) userdata;
    if (controller != current_instance) { return; }
    if (!controller->base.managed->obj_created) { return; }
    lv_btnmatrix_clear_btn_ctrl_all(controller->actions, LV_BTNMATRIX_CTRL_DISABLED);
    const SERVER_STATE *state = pcmanager_state(pcmanager, &controller->uuid);
    assert(state != NULL);
    if (state->code & SERVER_STATE_ONLINE || result != GS_OK) { return; }
    pcmanager_request_update(pcmanager, &controller->uuid, host_info_cb, controller);
}

static void update_view_state(apps_fragment_t *controller) {
    if (controller != current_instance) { return; }
    if (!controller->base.managed->obj_created || controller->base.managed->destroying_obj) { return; }
    launcher_fragment_t *parent_controller = (launcher_fragment_t *) lv_fragment_get_parent(&controller->base);
    parent_controller->detail_changing = true;
    const SERVER_STATE *state = pcmanager_state(pcmanager, &controller->uuid);
    assert(state != NULL);
    switch (state->code) {
        case SERVER_STATE_NONE:
        case SERVER_STATE_QUERYING: {
            // waiting to load server info
            show_progress(controller);
            break;
        }
        case SERVER_STATE_AVAILABLE: {
            switch (apploader_state(controller->apploader)) {
                case APPLOADER_STATE_LOADING: {
                    // is loading apps
                    if (controller->apploader_apps) {
                        break;
                    }
                    show_progress(controller);
                    break;
                }
                case APPLOADER_STATE_ERROR: {
                    // apploader has error
                    show_error(controller, locstr("Failed to load apps"),
                               locstr("Press \"Retry\" to load again.\n\nRestart your computer if error persists."),
                               controller->apploader_error);

                    set_actions(controller, action_labels_error, action_callbacks_error);
                    lv_group_focus_obj(controller->actions);
                    lv_obj_add_state(controller->actions, LV_STATE_FOCUS_KEY);
                    break;
                }
                case APPLOADER_STATE_IDLE: {
                    // has apps
                    show_ok(controller);
                    break;
                }
            }
            break;
        }
        case SERVER_STATE_NOT_PAIRED: {
            show_error(controller, locstr("Not paired"),
                       locstr("Press \"Pair\" to pair this host with your account."),
                       NULL);

            set_actions(controller, actions_unpaired, action_callbacks_unpaired);
            lv_group_focus_obj(controller->actions);
            lv_obj_add_state(controller->actions, LV_STATE_FOCUS_KEY);
            break;
        }
        case SERVER_STATE_ERROR: {
            // server has error
            show_error(controller, locstr("Host error"),
                       locstr("Press \"Retry\" to load again.\n\nRestart your computer if error persists."),
                       state->error.errmsg);

            set_actions(controller, action_labels_error, action_callbacks_error);
            lv_group_focus_obj(controller->actions);
            lv_obj_add_state(controller->actions, LV_STATE_FOCUS_KEY);
            break;
        }
        case SERVER_STATE_OFFLINE: {
            // server has error
            show_error(controller, locstr("Offline"),
                       locstr("Press \"Wake\" to send Wake-on-LAN packet to turn the computer on if it supports this feature, "
                              "press \"Retry\" to connect again.\n\n"
                              "Try restart your computer if these doesn't work."),
                       NULL);

            set_actions(controller, action_labels_offline, action_callbacks_offline);
            lv_btnmatrix_clear_btn_ctrl(controller->actions, 0, LV_BTNMATRIX_CTRL_DISABLED);
            lv_group_focus_obj(controller->actions);
            lv_obj_add_state(controller->actions, LV_STATE_FOCUS_KEY);
            break;
        }
        default: {
            break;
        }
    }
    parent_controller->detail_changing = false;
}

static void show_progress(apps_fragment_t *fragment) {
    lv_obj_add_flag(fragment->applist, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(fragment->apperror, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(fragment->appload, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(fragment->actions, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_state(fragment->actions, LV_STATE_DISABLED);

    lv_group_focus_obj(fragment->appload);
    lv_obj_add_state(fragment->appload, LV_STATE_FOCUS_KEY);
}

static void show_ok(apps_fragment_t *fragment) {
    lv_obj_clear_flag(fragment->applist, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(fragment->apperror, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(fragment->appload, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(fragment->actions, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_state(fragment->actions, LV_STATE_DISABLED);

    if (lv_group_get_focused(lv_obj_get_group(fragment->applist)) != fragment->applist) {
        lv_group_focus_obj(fragment->applist);
    }
}

static void show_error(apps_fragment_t *fragment, const char *title, const char *hint, const char *detail) {
    lv_obj_add_flag(fragment->appload, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(fragment->apperror, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(fragment->applist, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(fragment->actions, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_state(fragment->actions, LV_STATE_DISABLED);

    lv_obj_set_style_opa(fragment->errordetail, detail ? LV_OPA_100 : LV_OPA_0, 0);
    lv_label_set_text_static(fragment->errortitle, title);
    lv_label_set_text_static(fragment->errorhint, hint);
}

static void appload_started(void *userdata) {
    apps_fragment_t *fragment = userdata;
    update_view_state(fragment);
}

static void appload_loaded(apploader_list_t *apps, void *userdata) {
    apps_fragment_t *fragment = userdata;
    if (!fragment->base.managed->obj_created || fragment->base.managed->destroying_obj) {
        return;
    }
    int num_changes = -1;
    lv_gridview_data_change_t *changes = apps_list_detect_change(fragment->apploader_apps, apps, &num_changes);
    if (num_changes != 0) {
        lv_gridview_focus(fragment->applist, -1);
    }
    lv_gridview_set_data_advanced(fragment->applist, apps, changes, num_changes);
    if (changes != NULL) {
        free(changes);
    }
    apploader_list_free(fragment->apploader_apps);
    fragment->apploader_apps = apps;
    update_view_state(fragment);

    if (fragment->def_app > 0 && !fragment->def_app_launched) {
        fragment->def_app_launched = true;
        const apploader_item_t *app = NULL;
        for (int i = 0; i < apps->count; ++i) {
            if (apps->items[i].base.id == fragment->def_app) {
                app = &apps->items[i];
                break;
            }
        }
        if (app != NULL) {
            launcher_launch_game(fragment, app);
        }
    }
}

static void appload_errored(int code, const char *error, void *userdata) {
    LV_UNUSED(code);
    apps_fragment_t *fragment = userdata;
    fragment->apploader_error = error;
    update_view_state(fragment);
}

static void appitem_bind(apps_fragment_t *controller, lv_obj_t *item, apploader_item_t *app) {
    appitem_viewholder_t *holder = lv_obj_get_user_data(item);

    coverloader_display(controller->coverloader, &controller->uuid, app->base.id, item,
                        controller->col_width, controller->col_height);
    lv_label_set_text(holder->title, app->base.name);

    assert(controller->node);
    int current_id = pcmanager_node_current_app(controller->node);
    if (current_id == app->base.id) {
        lv_obj_clear_flag(holder->play_indicator, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(holder->play_indicator, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_set_style_opa(item, app->hidden ? LV_OPA_50 : LV_OPA_COVER, 0);
    holder->app_id = app->base.id;
}

static void item_click_cb(lv_event_t *event) {
    apps_fragment_t *controller = lv_event_get_user_data(event);
    lv_obj_t *target = lv_event_get_target(event);
    lv_obj_t *target_parent = lv_obj_get_parent(target);
    if (target_parent != controller->applist) {
        return;
    }
    appitem_viewholder_t *holder = (appitem_viewholder_t *) lv_obj_get_user_data(target);
    int current_app = pcmanager_server_current_app(pcmanager, &controller->uuid);
    if (current_app != 0) {
        if (holder->app_id == current_app) {
            open_context_menu(controller, holder);
        }
        return;
    }
    const apploader_item_t *item = apploader_list_item_by_id(controller->apploader_apps, holder->app_id);
    assert(item != NULL);
    launcher_launch_game(holder->controller, item);
}

static void item_longpress_cb(lv_event_t *event) {
    apps_fragment_t *controller = lv_event_get_user_data(event);
    lv_obj_t *target = lv_event_get_target(event);
    lv_obj_t *target_parent = lv_obj_get_parent(target);
    if (target_parent != controller->applist) {
        return;
    }
    lv_event_send(target, LV_EVENT_RELEASED, lv_event_get_indev(event));
    open_context_menu(controller, (appitem_viewholder_t *) lv_obj_get_user_data(target));
}

static void launcher_launch_game(apps_fragment_t *controller, const apploader_item_t *app) {
    LV_ASSERT(app->base.id != 0);
    streaming_scene_arg_t args = {
            .global = controller->global,
            .uuid = controller->uuid,
            .app = app->base,
    };
    app_ui_t *ui = &controller->global->ui;
    lv_fragment_t *fragment = lv_fragment_create(&streaming_controller_class, &args);
    lv_obj_t *const *container = lv_fragment_get_container(lv_fragment_manager_get_top(ui->fm));
    lv_fragment_manager_push(ui->fm, fragment, container);
}

static void launcher_toggle_fav(apps_fragment_t *controller, const apploader_item_t *app) {
    pcmanager_favorite_app(pcmanager, &controller->uuid, app->base.id, !app->fav);
    apploader_load(controller->apploader);
}

static void launcher_toggle_hidden(apps_fragment_t *controller, const apploader_item_t *app) {
    pcmanager_set_app_hidden(pcmanager, &controller->uuid, app->base.id, !app->hidden);
    controller->show_hidden_apps = true;
    apploader_load(controller->apploader);
}

static void launcher_quit_game(apps_fragment_t *controller) {
    controller->quit_progress = progress_dialog_create(locstr("Quitting game..."));
    pcmanager_quitapp(pcmanager, &controller->uuid, quitgame_cb, controller);
}

static int adapter_item_count(lv_obj_t *grid, void *data) {
    if (data == NULL) { return 0; }
    apps_fragment_t *controller = lv_obj_get_user_data(grid);
    apploader_list_t *list = data;
    // LVGL can only display up to 255 rows/columns, but I don't think anyone has library that big (1275 items)
    int count = LV_MIN(list->count, 255 * controller->col_count);
    if (!controller->show_hidden_apps) {
        for (int i = 0; i < count; i++) {
            if (list->items[i].hidden) {
                count = i;
                break;
            }
        }
    }
    return count;
}

static lv_obj_t *adapter_create_view(lv_obj_t *parent) {
    apps_fragment_t *controller = lv_obj_get_user_data(parent);
    return appitem_view(controller, parent);
}

static void adapter_bind_view(lv_obj_t *grid, lv_obj_t *item_view, void *data, int position) {
    apps_fragment_t *controller = lv_obj_get_user_data(grid);
    apploader_list_t *list = data;
    appitem_bind(controller, item_view, &list->items[position]);

    // IDE seems to be pretty confused...
    LV_UNUSED(list);
    LV_UNUSED(position);
}


static void applist_focus_enter(lv_event_t *event) {
    if (event->target != event->current_target) { return; }
    apps_fragment_t *controller = lv_event_get_user_data(event);
    lv_gridview_focus(controller->applist, controller->focus_backup);
}

static void applist_focus_leave(lv_event_t *event) {
    if (event->target != event->current_target) { return; }
    apps_fragment_t *controller = lv_event_get_user_data(event);
    controller->focus_backup = lv_gridview_get_focused_index(controller->applist);
    lv_gridview_focus(controller->applist, -1);
}

static void quitgame_cb(int result, const char *error, const uuidstr_t *uuid, void *userdata) {
    apps_fragment_t *controller = userdata;
    if (controller->quit_progress) {
        lv_msgbox_close(controller->quit_progress);
        controller->quit_progress = NULL;
    }
    lv_gridview_rebind(controller->applist);
    if (result == GS_OK) {
        return;
    }
    static const char *btn_texts[] = {translatable("OK"), ""};
    lv_obj_t *dialog = lv_msgbox_create_i18n(NULL, locstr("Unable to quit game"),
                                             locstr("Please make sure you are quitting with the same client."),
                                             btn_texts, false);
    lv_obj_add_event_cb(dialog, quit_dialog_cb, LV_EVENT_VALUE_CHANGED, controller);
    lv_obj_center(dialog);
}

static void quit_dialog_cb(lv_event_t *event) {
    lv_obj_t *dialog = lv_event_get_current_target(event);
    lv_msgbox_close_async(dialog);
}

static void actions_click_cb(lv_event_t *event) {
    apps_fragment_t *controller = lv_event_get_user_data(event);
    uint16_t index = lv_btnmatrix_get_selected_btn(controller->actions);
    const action_cb_t *actions = lv_obj_get_user_data(controller->actions);
    actions[index](controller, controller->actions, index);
}

static void action_cb_wol(apps_fragment_t *controller, lv_obj_t *buttons, uint16_t index) {
    LV_UNUSED(index);
    lv_btnmatrix_set_btn_ctrl_all(buttons, LV_BTNMATRIX_CTRL_DISABLED);
    pcmanager_send_wol(pcmanager, &controller->uuid, send_wol_cb, controller);
}

static void action_cb_host_reload(apps_fragment_t *controller, lv_obj_t *buttons, uint16_t index) {
    LV_UNUSED(index);
    lv_btnmatrix_set_btn_ctrl_all(buttons, LV_BTNMATRIX_CTRL_DISABLED);
    pcmanager_request_update(pcmanager, &controller->uuid, host_info_cb, controller);
}

static void action_cb_pair(apps_fragment_t *controller, lv_obj_t *buttons, uint16_t index) {
    LV_UNUSED(buttons);
    LV_UNUSED(index);
    pair_dialog_open(&controller->uuid);
}

static void open_context_menu(apps_fragment_t *fragment, appitem_viewholder_t *holder) {
    const apploader_item_t *app = apploader_list_item_by_id(fragment->apploader_apps, holder->app_id);
    assert(app != NULL);
    lv_obj_t *msgbox = lv_msgbox_create(NULL, app->base.name, NULL, NULL, false);
    lv_obj_set_user_data(msgbox, (void *) holder);
    lv_obj_t *content = lv_msgbox_get_content(msgbox);
    lv_obj_add_flag(content, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);

    lv_obj_add_event_cb(content, context_menu_cancel_cb, LV_EVENT_CANCEL, fragment);
    lv_obj_add_event_cb(content, context_menu_click_cb, LV_EVENT_SHORT_CLICKED, fragment);

    int currentId = pcmanager_server_current_app(pcmanager, &fragment->uuid);
    if (!currentId || currentId == app->base.id) {
        lv_obj_t *start_btn = lv_list_add_btn(content, NULL,
                                              currentId == app->base.id ? locstr("Resume streaming")
                                                                        : locstr("Start streaming"));
        lv_obj_add_flag(start_btn, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_set_user_data(start_btn, launcher_launch_game);
    }

    if (currentId) {
        lv_obj_t *quit_btn = lv_list_add_btn(content, NULL, locstr("Stop streaming"));
        lv_obj_add_flag(quit_btn, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_set_user_data(quit_btn, launcher_quit_game);
    }
    lv_obj_t *fav_btn = lv_list_add_btn(content, NULL, app->fav ? locstr("Unstar") : locstr("Star"));
    lv_obj_add_flag(fav_btn, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_user_data(fav_btn, launcher_toggle_fav);

    lv_obj_t *hide_btn = lv_list_add_btn(content, NULL, app->hidden ? locstr("Unhide") : locstr("Hide"));
    lv_obj_add_flag(hide_btn, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_user_data(hide_btn, launcher_toggle_hidden);

    lv_obj_t *info_btn = lv_list_add_btn(content, NULL, locstr("Info"));
    lv_obj_add_flag(info_btn, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_user_data(info_btn, app_detail_dialog);

    lv_obj_t *cancel_btn = lv_list_add_btn(content, NULL, locstr("Cancel"));
    lv_obj_add_flag(cancel_btn, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_center(msgbox);
}

static void context_menu_cancel_cb(lv_event_t *e) {
    lv_obj_t *target = lv_event_get_target(e);
    if (target->parent != lv_event_get_current_target(e)) { return; }
    lv_msgbox_close(lv_event_get_current_target(e)->parent);
}

static void context_menu_click_cb(lv_event_t *e) {
    lv_obj_t *target = lv_event_get_target(e);
    lv_obj_t *current_target = lv_event_get_current_target(e);
    if (target->parent != current_target) { return; }
    lv_obj_t *mbox = lv_event_get_current_target(e)->parent;
    apps_fragment_t *self = lv_event_get_user_data(e);
    appitem_viewholder_t *holder = lv_obj_get_user_data(mbox);
    const apploader_item_t *app = apploader_list_item_by_id(self->apploader_apps, holder->app_id);
    assert(app != NULL);
    if (lv_obj_get_user_data(target) == launcher_quit_game) {
        launcher_quit_game(self);
    } else if (lv_obj_get_user_data(target) == launcher_launch_game) {
        launcher_launch_game(self, app);
    } else if (lv_obj_get_user_data(target) == launcher_toggle_fav) {
        launcher_toggle_fav(self, app);
    } else if (lv_obj_get_user_data(target) == launcher_toggle_hidden) {
        launcher_toggle_hidden(self, app);
    } else if (lv_obj_get_user_data(target) == app_detail_dialog) {
        app_detail_dialog(self, app);
    }
    lv_msgbox_close_async(mbox);
}

static void app_detail_dialog(apps_fragment_t *fragment, const apploader_item_t *app) {
    LV_UNUSED(fragment);
    static const char *btn_txts[] = {translatable("OK"), ""};
    lv_obj_t *msgbox = lv_msgbox_create_i18n(NULL, app->base.name, "text", btn_txts, false);
    lv_obj_t *msgobj = lv_msgbox_get_text(msgbox);
    lv_label_set_text_fmt(msgobj,
                          "ID: %d\n"
                          "Support HDR: %s",
                          app->base.id, app->base.hdr ? "Yes" : "No");
    lv_obj_center(msgbox);
    lv_obj_add_event_cb(msgbox, app_detail_click_cb, LV_EVENT_CLICKED, NULL);
}

static void app_detail_click_cb(lv_event_t *event) {
    lv_msgbox_close(lv_event_get_current_target(event));
}

static void set_actions(apps_fragment_t *controller, const char **labels, const action_cb_t *callbacks) {
    lv_btnmatrix_set_map(controller->actions, labels);
    int num_actions = 0;
    for (int i = 0; labels[i][0] != '\0'; i++) {
        num_actions++;
    }
    lv_obj_set_style_min_width(controller->actions, LV_PCT(20 * num_actions), 0);
    lv_obj_set_user_data(controller->actions, (void *) callbacks);
}


static lv_gridview_data_change_t *apps_list_detect_change(const apploader_list_t *old_list,
                                                          const apploader_list_t *new_list, int *num_changes) {
    if (old_list == NULL && new_list == NULL) {
        *num_changes = 0;
        return NULL;
    } else if ((old_list != NULL) != (new_list != NULL)) {
        *num_changes = -1;
        return NULL;
    } else if (old_list->count != new_list->count) {
        *num_changes = -1;
        return NULL;
    }
    for (int i = 0; i < old_list->count; i++) {
        if (old_list->items[i].base.id != new_list->items[i].base.id) {
            *num_changes = -1;
            return NULL;
        }
    }
    *num_changes = 0;
    return NULL;
}