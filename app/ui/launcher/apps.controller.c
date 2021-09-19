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

typedef struct {
    lv_obj_controller_t base;
    apploader_t *apploader;
    coverloader_t *coverloader;
    PSERVER_LIST node;
    lv_obj_t *applist, *appload, *apperror;
    lv_obj_t *errorlabel;

    appitem_styles_t appitem_style;
    int col_count;
    lv_coord_t col_width, col_height;
    int focus_backup;
} apps_controller_t;


static lv_obj_t *apps_view(lv_obj_controller_t *self, lv_obj_t *parent);

static void on_view_created(lv_obj_controller_t *self, lv_obj_t *view);

static void on_destroy_view(lv_obj_controller_t *self, lv_obj_t *view);

static bool on_event(lv_obj_controller_t *self, int which, void *data1, void *data2);

static void host_info_cb(const pcmanager_resp_t *resp, void *userdata);

static void on_host_updated(const pcmanager_resp_t *resp, void *userdata);

static void on_host_removed(const pcmanager_resp_t *resp, void *userdata);

static void item_click_cb(lv_event_t *event);

static void launcher_launch_game(apps_controller_t *controller, const APP_LIST *app);

static void launcher_quit_game(apps_controller_t *controller);

static void applist_focus_enter(lv_event_t *event);

static void applist_focus_leave(lv_event_t *event);

static void update_view_state(apps_controller_t *controller);

static void appitem_bind(apps_controller_t *controller, lv_obj_t *item, APP_LIST *app);

static int adapter_item_count(lv_obj_t *, void *data);

static lv_obj_t *adapter_create_view(lv_obj_t *parent);

static void adapter_bind_view(lv_obj_t *, lv_obj_t *, void *data, int position);

static int adapter_item_id(lv_obj_t *, void *data, int position);

static void quitgame_cb(const pcmanager_resp_t *resp, void *userdata);

static void apps_controller_ctor(lv_obj_controller_t *self, void *args);

static void apps_controller_dtor(lv_obj_controller_t *self);

static void appload_cb(apploader_t *loader, void *userdata);

static void quit_dialog_cb(lv_event_t *event);

static void update_grid_config(apps_controller_t *controller);

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
    apploader_destroy(controller->apploader);
}

static lv_obj_t *apps_view(lv_obj_controller_t *self, lv_obj_t *parent) {
    apps_controller_t *controller = (apps_controller_t *) self;

    lv_obj_t *applist = lv_gridview_create(parent);
    lv_obj_add_flag(applist, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_style_pad_all(applist, lv_dpx(24), 0);
    lv_obj_set_style_pad_gap(applist, lv_dpx(24), 0);
    lv_obj_set_style_radius(applist, 0, 0);
    lv_obj_set_style_border_side(applist, LV_BORDER_SIDE_NONE, 0);
    lv_obj_set_style_bg_opa(applist, 0, 0);
    lv_obj_set_size(applist, LV_PCT(100), LV_PCT(100));
    lv_grid_set_adapter(applist, &apps_adapter);
    lv_obj_t *appload = lv_spinner_create(parent, 1000, 60);
    lv_obj_set_size(appload, lv_dpx(60), lv_dpx(60));
    lv_obj_center(appload);
    lv_obj_t *apperror = lv_obj_create(parent);
    lv_obj_set_size(apperror, LV_PCT(80), LV_PCT(60));
    lv_obj_center(apperror);
    lv_obj_t *errorlabel = lv_label_create(apperror);
    lv_obj_center(errorlabel);

    controller->applist = applist;
    controller->appload = appload;
    controller->apperror = apperror;
    controller->errorlabel = errorlabel;
    return NULL;
}

static void on_view_created(lv_obj_controller_t *self, lv_obj_t *view) {
    apps_controller_t *controller = (apps_controller_t *) self;
    controller->coverloader = coverloader_new();
    pcmanager_register_listener(pcmanager, &pc_listeners, controller);
    lv_obj_t *applist = controller->applist;
    lv_obj_add_event_cb(applist, item_click_cb, LV_EVENT_CLICKED, controller);
    lv_obj_add_event_cb(applist, applist_focus_enter, LV_EVENT_FOCUSED, controller);
    lv_obj_add_event_cb(applist, applist_focus_leave, LV_EVENT_DEFOCUSED, controller);
    lv_obj_add_event_cb(applist, applist_focus_leave, LV_EVENT_LEAVE, controller);

    update_grid_config(controller);
    lv_obj_set_user_data(controller->applist, controller);

    pcmanager_request_update(pcmanager, controller->node->server, host_info_cb, controller);
    apploader_load(controller->apploader, appload_cb, controller);
    update_view_state(controller);
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
    apps_controller_t *controller = (apps_controller_t *) self;

    pcmanager_unregister_listener(pcmanager, &pc_listeners);
    coverloader_destroy(controller->coverloader);
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
}

static void update_view_state(apps_controller_t *controller) {
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
                lv_label_set_text_static(controller->errorlabel, "Not paired");
            } else if (controller->apploader->status == APPLOADER_STATUS_LOADING) {
                // is loading apps
                lv_obj_add_flag(applist, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(apperror, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(appload, LV_OBJ_FLAG_HIDDEN);
            } else if (controller->apploader->code == 0) {
                // has apps
                lv_obj_clear_flag(applist, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(apperror, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(appload, LV_OBJ_FLAG_HIDDEN);
                lv_grid_set_data(controller->applist, controller->apploader->apps);
            } else {
                // apploader has error
                lv_obj_add_flag(appload, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(apperror, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(applist, LV_OBJ_FLAG_HIDDEN);
                lv_label_set_text_static(controller->errorlabel, "Failed to load apps");
            }
            break;
        }
        case SERVER_STATE_ERROR: {
            // server has error
            lv_obj_add_flag(appload, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(apperror, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(applist, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text_static(controller->errorlabel, "Failed to load server info");
        }
        case SERVER_STATE_OFFLINE: {
            // server has error
            lv_obj_add_flag(appload, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(apperror, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(applist, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text_static(controller->errorlabel, "Host is offline");
            break;
        }
        default: {
            break;
        }
    }
}

static void appitem_bind(apps_controller_t *controller, lv_obj_t *item, APP_LIST *app) {
    appitem_viewholder_t *holder = lv_obj_get_user_data(item);

    coverloader_display(controller->coverloader, controller->node, app->id, item, controller->col_width,
                        controller->col_height);
    lv_label_set_text(holder->title, app->name);

    if (controller->node->server->currentGame == app->id) {
        lv_obj_clear_flag(holder->play_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(holder->close_btn, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(holder->play_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(holder->close_btn, LV_OBJ_FLAG_HIDDEN);
    }
    holder->app = app;
}

static void item_click_cb(lv_event_t *event) {
    apps_controller_t *controller = lv_event_get_user_data(event);
    lv_obj_t *target = lv_event_get_target(event);
    lv_obj_t *target_parent = lv_obj_get_parent(target);
    if (target_parent != controller->applist) {
        if (lv_obj_get_parent(target_parent) == controller->applist) {
            appitem_viewholder_t *holder = lv_obj_get_user_data(target_parent);
            if (target == holder->play_btn) {
                launcher_launch_game(controller, holder->app);
            } else if (target == holder->close_btn) {
                launcher_quit_game(controller);
            }
        }
        return;
    }
    appitem_viewholder_t *holder = (appitem_viewholder_t *) lv_obj_get_user_data(target);
    launcher_launch_game(controller, holder->app);
}

static void launcher_launch_game(apps_controller_t *controller, const APP_LIST *app) {
    streaming_scene_arg_t args = {
            .server = controller->node->server,
            .app = app,
    };
    lv_controller_manager_push(app_uimanager, &streaming_controller_class, &args);
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
    return appitem_view(parent, &controller->appitem_style);
}

static void adapter_bind_view(lv_obj_t *grid, lv_obj_t *item_view, void *data, int position) {
    APP_LIST *apps = (APP_LIST *) data;
    apps_controller_t *controller = lv_obj_get_user_data(grid);
    appitem_bind(controller, item_view, &apps[position]);
}

static int adapter_item_id(lv_obj_t *grid, void *data, int position) {
    APP_LIST *apps = (APP_LIST *) data;
    return apps[position].id;
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
    update_view_state(controller);
}

static void quit_dialog_cb(lv_event_t *event) {
    lv_obj_t *dialog = lv_event_get_current_target(event);
    lv_msgbox_close_async(dialog);
}