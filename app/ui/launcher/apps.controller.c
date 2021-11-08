//
// Created by Mariotaku on 2021/08/31.
//

#include <errors.h>
#include <util/user_event.h>
#include "app.h"

#include "coverloader.h"
#include "backend/apploader/apploader.h"
#include "lvgl/ext/lv_gridview.h"
#include "lvgl/lv_ext_utils.h"
#include "ui/streaming/streaming.controller.h"
#include "appitem.view.h"
#include "launcher.controller.h"
#include "apps.controller.h"


static lv_obj_t *apps_view(lv_obj_controller_t *self, lv_obj_t *parent);

static void on_view_created(lv_obj_controller_t *self, lv_obj_t *view);

static void on_destroy_view(lv_obj_controller_t *self, lv_obj_t *view);

static bool on_event(lv_obj_controller_t *self, int which, void *data1, void *data2);

static void host_info_cb(const pcmanager_resp_t *resp, void *userdata);

static void send_wol_cb(const pcmanager_resp_t *resp, void *userdata);

static void on_host_updated(const pcmanager_resp_t *resp, void *userdata);

static void on_host_removed(const pcmanager_resp_t *resp, void *userdata);

static void item_click_cb(lv_event_t *event);

static void item_longpress_cb(lv_event_t *event);

static void launcher_launch_game(apps_controller_t *controller, const apploader_item_t *app);

static void launcher_launch_game_async(appitem_viewholder_t *holder);

static void launcher_toggle_fav(apps_controller_t *controller, const apploader_item_t *app);

static void launcher_quit_game(apps_controller_t *controller);

static void applist_focus_enter(lv_event_t *event);

static void applist_focus_leave(lv_event_t *event);

static void update_view_state(apps_controller_t *controller);

static void appitem_bind(apps_controller_t *controller, lv_obj_t *item, apploader_item_t *app);

static int adapter_item_count(lv_obj_t *, void *data);

static lv_obj_t *adapter_create_view(lv_obj_t *parent);

static void adapter_bind_view(lv_obj_t *, lv_obj_t *, void *data, int position);

static int adapter_item_id(lv_obj_t *, void *data, int position);

static void quitgame_cb(const pcmanager_resp_t *resp, void *userdata);

static void apps_controller_ctor(lv_obj_controller_t *self, void *args);

static void apps_controller_dtor(lv_obj_controller_t *self);

static void appload_cb(apploader_t *loader, void *userdata);

static void quit_dialog_cb(lv_event_t *event);

static void actions_click_cb(lv_event_t *event);

static void update_grid_config(apps_controller_t *controller);

static void open_context_menu(apps_controller_t *controller, apploader_item_t *app);

static void context_menu_key_cb(lv_event_t *event);

static void context_menu_click_cb(lv_event_t *event);

static void app_detail_dialog(apps_controller_t *controller, apploader_item_t *app);

static void app_detail_click_cb(lv_event_t *event);

static apps_controller_t *current_instance = NULL;

const static lv_grid_adapter_t apps_adapter = {
        .item_count = adapter_item_count,
        .create_view = adapter_create_view,
        .bind_view = adapter_bind_view,
        .item_id = adapter_item_id,
};
const static pcmanager_listener_t pc_listeners = {
        .updated = on_host_updated,
        .removed = on_host_removed,
};

const lv_obj_controller_class_t apps_controller_class = {
        .constructor_cb = apps_controller_ctor,
        .destructor_cb = apps_controller_dtor,
        .create_obj_cb = apps_view,
        .obj_created_cb = on_view_created,
        .obj_deleted_cb = on_destroy_view,
        .event_cb = on_event,
        .instance_size = sizeof(apps_controller_t),
};

static void apps_controller_ctor(lv_obj_controller_t *self, void *args) {
    apps_controller_t *controller = (apps_controller_t *) self;
    controller->node = args;
    controller->apploader = apploader_new(controller->node);

    appitem_style_init(&controller->appitem_style);
}

static void apps_controller_dtor(lv_obj_controller_t *self) {
    apps_controller_t *controller = (apps_controller_t *) self;
    appitem_style_deinit(&controller->appitem_style);
    apploader_unref(controller->apploader);
}

static lv_obj_t *apps_view(lv_obj_controller_t *self, lv_obj_t *parent) {
    apps_controller_t *controller = (apps_controller_t *) self;

    lv_obj_t *applist = controller->applist = lv_gridview_create(parent);
    lv_obj_add_flag(applist, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_style_pad_all(applist, lv_dpx(24), 0);
    lv_obj_set_style_pad_gap(applist, lv_dpx(24), 0);
    lv_obj_set_style_radius(applist, 0, 0);
    lv_obj_set_style_border_side(applist, LV_BORDER_SIDE_NONE, 0);
    lv_obj_set_style_bg_opa(applist, 0, 0);
    lv_obj_set_size(applist, LV_PCT(100), LV_PCT(100));
    lv_grid_set_adapter(applist, &apps_adapter);
    lv_obj_t *appload = controller->appload = lv_spinner_create(parent, 1000, 60);
    lv_obj_set_size(appload, lv_dpx(60), lv_dpx(60));
    lv_obj_center(appload);
    lv_obj_t *apperror = controller->apperror = lv_obj_create(parent);
    lv_obj_add_flag(apperror, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_size(apperror, LV_PCT(80), LV_PCT(60));
    lv_obj_center(apperror);
    lv_obj_set_flex_flow(apperror, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(apperror, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
    lv_obj_t *errortitle = controller->errortitle = lv_label_create(apperror);
    lv_obj_set_width(errortitle, LV_PCT(100));
    lv_obj_set_style_text_font(errortitle, lv_theme_get_font_large(apperror), 0);
    lv_obj_t *errorlabel = controller->errorlabel = lv_label_create(apperror);
    lv_obj_set_width(errorlabel, LV_PCT(100));
    lv_obj_set_flex_grow(errorlabel, 1);
    controller->actions = lv_btnmatrix_create(apperror);
    lv_obj_set_style_border_side(controller->actions, LV_BORDER_SIDE_NONE, 0);
    lv_obj_set_size(controller->actions, LV_DPX(500), LV_SIZE_CONTENT);
    lv_obj_set_style_min_height(controller->actions, LV_DPX(80), 0);
    lv_obj_set_style_max_width(controller->actions, LV_PCT(100), 0);
    lv_btnmatrix_set_btn_ctrl_all(controller->actions, LV_BTNMATRIX_CTRL_CLICK_TRIG | LV_BTNMATRIX_CTRL_NO_REPEAT);
    static const char *actions_map[] = {"Send Wake-On-LAN", "Retry", ""};
    lv_btnmatrix_set_map(controller->actions, actions_map);
    lv_obj_add_flag(controller->actions, LV_OBJ_FLAG_EVENT_BUBBLE);
    return NULL;
}

static void on_view_created(lv_obj_controller_t *self, lv_obj_t *view) {
    apps_controller_t *controller = (apps_controller_t *) self;
    controller->coverloader = coverloader_new();
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

    if (controller->node->state.code == SERVER_STATE_NONE) {
        pcmanager_request_update(pcmanager, controller->node->server, host_info_cb, controller);
        apploader_load(controller->apploader, appload_cb, controller);
    } else if (controller->node->state.code == SERVER_STATE_ONLINE) {
        apploader_load(controller->apploader, appload_cb, controller);
    }
    update_view_state(controller);

    current_instance = controller;
}

static void update_grid_config(apps_controller_t *controller) {
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
    lv_gridview_set_config(applist, col_count, row_height);
}

static void on_destroy_view(lv_obj_controller_t *self, lv_obj_t *view) {
    current_instance = NULL;
    apps_controller_t *controller = (apps_controller_t *) self;

    pcmanager_unregister_listener(pcmanager, &pc_listeners);
    coverloader_unref(controller->coverloader);
}

static bool on_event(lv_obj_controller_t *self, int which, void *data1, void *data2) {
    apps_controller_t *controller = (apps_controller_t *) self;
    switch (which) {
        case USER_SIZE_CHANGED: {
            update_grid_config(controller);
            lv_grid_rebind(controller->applist);
            break;
        }
    }
    return false;
}

static void on_host_updated(const pcmanager_resp_t *resp, void *userdata) {
    apps_controller_t *controller = (apps_controller_t *) userdata;
    if (controller != current_instance) return;
    if (resp->server != controller->node->server) return;
    apploader_load(controller->apploader, appload_cb, controller);
    update_view_state(controller);
}

static void on_host_removed(const pcmanager_resp_t *resp, void *userdata) {
    apps_controller_t *controller = (apps_controller_t *) userdata;
    if (resp->server != controller->node->server) return;
    lv_obj_controller_pop((lv_obj_controller_t *) controller);
}

static void host_info_cb(const pcmanager_resp_t *resp, void *userdata) {
    apps_controller_t *controller = (apps_controller_t *) userdata;
    if (controller != current_instance) return;
    if (resp->state.code == SERVER_STATE_ONLINE && !controller->apploader->apps) {
        apploader_load(controller->apploader, appload_cb, controller);
    }
}

static void send_wol_cb(const pcmanager_resp_t *resp, void *userdata) {
    apps_controller_t *controller = (apps_controller_t *) userdata;
    if (controller != current_instance) return;
    lv_btnmatrix_clear_btn_ctrl_all(controller->actions, LV_BTNMATRIX_CTRL_DISABLED);
    if (controller->node->state.code == SERVER_STATE_ONLINE || resp->result.code != GS_OK) return;
    pcmanager_request_update(pcmanager, controller->node->server, host_info_cb, controller);
}

static void update_view_state(apps_controller_t *controller) {
    launcher_controller_t *parent_controller = (launcher_controller_t *) lv_controller_manager_parent(
            controller->base.manager);
    parent_controller->detail_changing = true;
    PSERVER_LIST node = controller->node;
    LV_ASSERT(node);
    lv_obj_t *applist = controller->applist;
    lv_obj_t *appload = controller->appload;
    lv_obj_t *apperror = controller->apperror;
    switch (node->state.code) {
        case SERVER_STATE_NONE:
        case SERVER_STATE_QUERYING: {
            // waiting to load server info
            lv_obj_add_flag(applist, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(apperror, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(appload, LV_OBJ_FLAG_HIDDEN);
            break;
        }
        case SERVER_STATE_ONLINE: {
            if (!node->server->paired) {
                lv_obj_add_flag(appload, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(apperror, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(applist, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(controller->actions, LV_OBJ_FLAG_HIDDEN);
                lv_label_set_text_static(controller->errortitle, "Unable to open games");
                lv_label_set_text_static(controller->errorlabel, "Not paired");
            } else if (controller->apploader->status == APPLOADER_STATUS_LOADING) {
                // is loading apps
                lv_obj_add_flag(applist, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(apperror, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(appload, LV_OBJ_FLAG_HIDDEN);
            } else if (controller->apploader->code != 0) {
                // apploader has error
                lv_obj_add_flag(appload, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(apperror, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(applist, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(controller->actions, LV_OBJ_FLAG_HIDDEN);
                lv_btnmatrix_set_btn_ctrl(controller->actions, 0, LV_BTNMATRIX_CTRL_DISABLED);
                lv_label_set_text_static(controller->errortitle, "Unable to open games");
                lv_label_set_text_static(controller->errorlabel, "Failed to load apps");
            } else {
                // has apps
                lv_obj_clear_flag(applist, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(apperror, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(appload, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(controller->actions, LV_OBJ_FLAG_HIDDEN);
                lv_grid_set_data(controller->applist, controller->apploader->apps);
                lv_group_focus_obj(controller->applist);
            }
            break;
        }
        case SERVER_STATE_ERROR: {
            // server has error
            lv_obj_add_flag(appload, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(apperror, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(applist, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(controller->actions, LV_OBJ_FLAG_HIDDEN);
            lv_btnmatrix_set_btn_ctrl(controller->actions, 0, LV_BTNMATRIX_CTRL_DISABLED);
            lv_label_set_text_static(controller->errortitle, "Unable to open games");
            lv_label_set_text_static(controller->errorlabel, "Failed to load server info");
            lv_group_focus_obj(controller->actions);
            lv_obj_add_state(controller->actions, LV_STATE_FOCUS_KEY);
            break;
        }
        case SERVER_STATE_OFFLINE: {
            // server has error
            lv_obj_add_flag(appload, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(apperror, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(applist, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(controller->actions, LV_OBJ_FLAG_HIDDEN);
            lv_btnmatrix_clear_btn_ctrl(controller->actions, 0, LV_BTNMATRIX_CTRL_DISABLED);
            lv_label_set_text_static(controller->errortitle, "Unable to open games");
            lv_label_set_text_static(controller->errorlabel, "Host is offline");
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

static void appitem_bind(apps_controller_t *controller, lv_obj_t *item, apploader_item_t *app) {
    appitem_viewholder_t *holder = lv_obj_get_user_data(item);

    coverloader_display(controller->coverloader, controller->node, app->base.id, item, controller->col_width,
                        controller->col_height);
    lv_label_set_text(holder->title, app->base.name);

    if (controller->node->server->currentGame == app->base.id) {
        lv_obj_clear_flag(holder->play_indicator, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(holder->play_indicator, LV_OBJ_FLAG_HIDDEN);
    }
    holder->app = app;
}

static void item_click_cb(lv_event_t *event) {
    apps_controller_t *controller = lv_event_get_user_data(event);
    lv_obj_t *target = lv_event_get_target(event);
    lv_obj_t *target_parent = lv_obj_get_parent(target);
    if (target_parent != controller->applist) {
        return;
    }
    appitem_viewholder_t *holder = (appitem_viewholder_t *) lv_obj_get_user_data(target);
    if (controller->node->server->currentGame) {
        if (holder->app->base.id == controller->node->server->currentGame) {
            open_context_menu(controller, holder->app);
        }
        return;
    }
    lv_async_call((lv_async_cb_t) launcher_launch_game_async, holder);
}

static void item_longpress_cb(lv_event_t *event) {
    apps_controller_t *controller = lv_event_get_user_data(event);
    lv_obj_t *target = lv_event_get_target(event);
    lv_obj_t *target_parent = lv_obj_get_parent(target);
    if (target_parent != controller->applist) {
        return;
    }
    lv_event_send(target, LV_EVENT_RELEASED, lv_event_get_indev(event));
    appitem_viewholder_t *holder = (appitem_viewholder_t *) lv_obj_get_user_data(target);
    open_context_menu(controller, holder->app);
}

static void launcher_launch_game(apps_controller_t *controller, const apploader_item_t *app) {
    streaming_scene_arg_t args = {
            .server = controller->node->server,
            .app = &app->base,
    };
    lv_controller_manager_push(app_uimanager, &streaming_controller_class, &args);
}

static void launcher_launch_game_async(appitem_viewholder_t *holder) {
    launcher_launch_game(holder->controller, holder->app);
}

static void launcher_toggle_fav(apps_controller_t *controller, const apploader_item_t *app) {
    pcmanager_favorite_app(controller->node, app->base.id, !app->fav);
    apploader_load(controller->apploader, appload_cb, controller);
}

static void launcher_quit_game(apps_controller_t *controller) {
    pcmanager_quitapp(pcmanager, controller->node->server, quitgame_cb, controller);
}

static int adapter_item_count(lv_obj_t *grid, void *data) {
    apps_controller_t *controller = lv_obj_get_user_data(grid);
    // LVGL can only display up to 255 rows/columns, but I don't think anyone has library that big (1275 items)
    return LV_MIN(controller->apploader->apps_count, 255 * controller->col_count);
}

static lv_obj_t *adapter_create_view(lv_obj_t *parent) {
    apps_controller_t *controller = lv_obj_get_user_data(parent);
    return appitem_view(controller, parent);
}

static void adapter_bind_view(lv_obj_t *grid, lv_obj_t *item_view, void *data, int position) {
    apploader_item_t *apps = (apploader_item_t *) data;
    apps_controller_t *controller = lv_obj_get_user_data(grid);
    appitem_bind(controller, item_view, &apps[position]);
}

static int adapter_item_id(lv_obj_t *grid, void *data, int position) {
    apploader_item_t *apps = (apploader_item_t *) data;
    return apps[position].base.id;
}


static void applist_focus_enter(lv_event_t *event) {
    if (event->target != event->current_target) return;
    apps_controller_t *controller = lv_event_get_user_data(event);
    lv_gridview_focus(controller->applist, controller->focus_backup);
}

static void applist_focus_leave(lv_event_t *event) {
    if (event->target != event->current_target) return;
    apps_controller_t *controller = lv_event_get_user_data(event);
    controller->focus_backup = lv_gridview_get_focused_index(controller->applist);
    lv_gridview_focus(controller->applist, -1);
}

static void quitgame_cb(const pcmanager_resp_t *resp, void *userdata) {
    apps_controller_t *controller = userdata;
    lv_grid_rebind(controller->applist);
    if (resp->result.code == GS_OK) {
        return;
    }
    static const char *btn_texts[] = {"OK", ""};
    lv_obj_t *dialog = lv_msgbox_create(NULL, "Unable to quit game",
                                        "Please make sure you are quitting with the same client.",
                                        btn_texts, false);
    lv_obj_add_event_cb(dialog, quit_dialog_cb, LV_EVENT_VALUE_CHANGED, controller);
    lv_obj_center(dialog);
}

static void appload_cb(apploader_t *loader, void *userdata) {
    apps_controller_t *controller = userdata;
    if (controller != current_instance) return;
    update_view_state(controller);
}

static void quit_dialog_cb(lv_event_t *event) {
    lv_obj_t *dialog = lv_event_get_current_target(event);
    lv_msgbox_close_async(dialog);
}

static void actions_click_cb(lv_event_t *event) {
    apps_controller_t *controller = lv_event_get_user_data(event);
    switch (lv_btnmatrix_get_selected_btn(controller->actions)) {
        case 0: {
            lv_btnmatrix_set_btn_ctrl_all(controller->actions, LV_BTNMATRIX_CTRL_DISABLED);
            pcmanager_send_wol(pcmanager, controller->node->server, send_wol_cb, controller);
            break;
        }
        case 1: {
            pcmanager_request_update(pcmanager, controller->node->server, host_info_cb, controller);
            break;
        }
    }
}

static void open_context_menu(apps_controller_t *controller, apploader_item_t *app) {
    lv_obj_t *msgbox = lv_msgbox_create(NULL, app->base.name, NULL, NULL, false);
    lv_obj_set_user_data(msgbox, app);
    lv_obj_t *content = lv_msgbox_get_content(msgbox);
    lv_obj_add_flag(content, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);

    lv_obj_add_event_cb(content, context_menu_key_cb, LV_EVENT_KEY, controller);
    lv_obj_add_event_cb(content, context_menu_click_cb, LV_EVENT_SHORT_CLICKED, controller);

    int currentId = controller->node->server->currentGame;

    if (!currentId || currentId == app->base.id) {
        lv_obj_t *start_btn = lv_list_add_btn(content, NULL, currentId == app->base.id ? "Resume" : "Start");
        lv_obj_add_flag(start_btn, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_set_user_data(start_btn, launcher_launch_game);
    }

    if (currentId) {
        lv_obj_t *quit_btn = lv_list_add_btn(content, NULL, "Quit Session");
        lv_obj_add_flag(quit_btn, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_set_user_data(quit_btn, launcher_quit_game);
    }
    lv_obj_t *fav_btn = lv_list_add_btn(content, NULL, app->fav ? "Remove star" : "Star");
    lv_obj_add_flag(fav_btn, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_user_data(fav_btn, launcher_toggle_fav);

    lv_obj_t *info_btn = lv_list_add_btn(content, NULL, "Info");
    lv_obj_add_flag(info_btn, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_user_data(info_btn, app_detail_dialog);

    lv_obj_t *cancel_btn = lv_list_add_btn(content, NULL, "Cancel");
    lv_obj_add_flag(cancel_btn, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_center(msgbox);
}

static void context_menu_key_cb(lv_event_t *e) {
    lv_obj_t *target = lv_event_get_target(e);
    if (target->parent != lv_event_get_current_target(e)) return;
    if (lv_event_get_key(e) == LV_KEY_ESC) {
        lv_msgbox_close_async(lv_event_get_current_target(e)->parent);
    }
}

static void context_menu_click_cb(lv_event_t *e) {
    lv_obj_t *target = lv_event_get_target(e);
    lv_obj_t *current_target = lv_event_get_current_target(e);
    if (target->parent != current_target) return;
    lv_obj_t *mbox = lv_event_get_current_target(e)->parent;
    apps_controller_t *controller = lv_event_get_user_data(e);
    if (lv_obj_get_user_data(target) == launcher_quit_game) {
        launcher_quit_game(controller);
    } else if (lv_obj_get_user_data(target) == launcher_launch_game) {
        launcher_launch_game(controller, lv_obj_get_user_data(mbox));
    } else if (lv_obj_get_user_data(target) == launcher_toggle_fav) {
        launcher_toggle_fav(controller, lv_obj_get_user_data(mbox));
    } else if (lv_obj_get_user_data(target) == app_detail_dialog) {
        app_detail_dialog(controller, lv_obj_get_user_data(mbox));
    }
    lv_msgbox_close_async(mbox);
}

static void app_detail_dialog(apps_controller_t *controller, apploader_item_t *app) {
    static const char *btn_txts[] = {"OK", ""};
    lv_obj_t *msgbox = lv_msgbox_create(NULL, app->base.name, "text", btn_txts, false);
    lv_obj_t *msgobj = lv_msgbox_get_text(msgbox);
    lv_label_set_text_fmt(msgobj,
                          "ID: %d\n"
                          "Support HDR: %s",
                          app->base.id, app->base.hdr ? "Yes" : "No");
    lv_obj_center(msgbox);
    lv_obj_add_event_cb(msgbox, app_detail_click_cb, LV_EVENT_VALUE_CHANGED, NULL);
}

static void app_detail_click_cb(lv_event_t *event) {
    lv_msgbox_close_async(lv_event_get_current_target(event));
}