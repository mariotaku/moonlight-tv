//
// Created by Mariotaku on 2021/08/31.
//

#include "app.h"

#include <malloc.h>
#include <string.h>
#include <ui/launcher/coverloader.h>
#include "lvgl/lv_gridview.h"
#include "lvgl/lv_ext_utils.h"
#include "backend/appmanager.h"
#include "ui/streaming/overlay.h"
#include "ui/streaming/streaming.controller.h"
#include "apps.controller.h"
#include "appitem.view.h"

typedef struct {
    ui_view_controller_t base;
    PCMANAGER_CALLBACKS _pcmanager_callbacks;
    APPMANAGER_CALLBACKS _appmanager_callbacks;
    coverloader_t *coverloader;
    PSERVER_LIST node;
    lv_obj_t *applist, *appload;

    appitem_styles_t appitem_style;
    int col_count;
    lv_coord_t col_width, col_height;
    int focus_backup;
} apps_controller_t;

static lv_obj_t *apps_view(ui_view_controller_t *self, lv_obj_t *parent);

static void on_view_created(ui_view_controller_t *self, lv_obj_t *view);

static void on_destroy_view(ui_view_controller_t *self, lv_obj_t *view);

static void on_host_updated(void *userdata, PPCMANAGER_RESP resp);

static void on_apps_updated(void *userdata, PSERVER_LIST node);

static void launcher_open_game(lv_event_t *event);

static void launcher_resume_game(lv_event_t *event);

static void launcher_quit_game(lv_event_t *event);

static void applist_focus_enter(lv_event_t *event);

static void applist_focus_leave(lv_event_t *event);

static void update_data(apps_controller_t *controller);

static void appitem_bind(apps_controller_t *controller, lv_obj_t *item, struct _APP_DLIST *app);

static int adapter_item_count(lv_obj_t *, void *data);

static lv_obj_t *adapter_create_view(lv_obj_t *parent);

static void adapter_bind_view(lv_obj_t *, lv_obj_t *, void *data, int position);

static int adapter_item_id(lv_obj_t *, void *data, int position);

const static lv_grid_adapter_t apps_adapter = {
        .item_count = adapter_item_count,
        .create_view = adapter_create_view,
        .bind_view = adapter_bind_view,
        .item_id = adapter_item_id,
};

ui_view_controller_t *apps_controller(void *args) {
    apps_controller_t *controller = lv_mem_alloc(sizeof(apps_controller_t));
    lv_memset_00(controller, sizeof(apps_controller_t));
    controller->base.create_view = apps_view;
    controller->base.view_created = on_view_created;
    controller->base.destroy_view = on_destroy_view;
    controller->base.destroy_controller = (void (*)(ui_view_controller_t *)) lv_mem_free;
    controller->_pcmanager_callbacks.added = NULL;
    controller->_pcmanager_callbacks.updated = on_host_updated;
    controller->_pcmanager_callbacks.userdata = controller;
    controller->_appmanager_callbacks.updated = on_apps_updated;
    controller->_appmanager_callbacks.userdata = controller;
    controller->node = args;

    appitem_style_init(&controller->appitem_style);
    return (ui_view_controller_t *) controller;
}

static lv_obj_t *apps_view(ui_view_controller_t *self, lv_obj_t *parent) {
    apps_controller_t *controller = (apps_controller_t *) self;

    lv_obj_t *applist = lv_gridview_create(parent);
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

    controller->applist = applist;
    controller->appload = appload;
    return NULL;
}

static void on_view_created(ui_view_controller_t *self, lv_obj_t *view) {
    apps_controller_t *controller = (apps_controller_t *) self;
    controller->coverloader = coverloader_new();
    appmanager_register_callbacks(&controller->_appmanager_callbacks);
    pcmanager_register_callbacks(&controller->_pcmanager_callbacks);
    lv_obj_t *applist = controller->applist;
    lv_obj_add_event_cb(applist, launcher_open_game, LV_EVENT_CLICKED, controller);
    lv_obj_add_event_cb(applist, applist_focus_enter, LV_EVENT_FOCUSED, controller);
    lv_obj_add_event_cb(applist, applist_focus_leave, LV_EVENT_DEFOCUSED, controller);
    lv_group_add_obj(lv_group_get_default(), applist);

    int col_count = 5;
    lv_coord_t applist_width = lv_measure_width(applist);
    lv_coord_t col_width = (applist_width - lv_obj_get_style_pad_left(applist, 0) -
                            lv_obj_get_style_pad_right(applist, 0) -
                            lv_obj_get_style_pad_column(applist, 0) * (col_count - 1)) / col_count;
    controller->col_count = col_count;
    controller->col_width = col_width;
    lv_coord_t row_height = col_width / 3 * 4;
    controller->col_height = row_height;
    lv_gridview_set_config(controller->applist, col_count, row_height);
    lv_obj_set_user_data(controller->applist, controller);

    update_data(controller);
    if (!controller->node->apps) {
        application_manager_load(controller->node);
    }
}

static void on_destroy_view(ui_view_controller_t *self, lv_obj_t *view) {
    apps_controller_t *controller = (apps_controller_t *) self;

    appmanager_unregister_callbacks(&controller->_appmanager_callbacks);
    pcmanager_unregister_callbacks(&controller->_pcmanager_callbacks);
    coverloader_destroy(controller->coverloader);
}

static void on_host_updated(void *userdata, PPCMANAGER_RESP resp) {
    apps_controller_t *controller = (apps_controller_t *) userdata;
    if (resp->server != controller->node->server) return;
    update_data(controller);
}

static void on_apps_updated(void *userdata, PSERVER_LIST node) {
    apps_controller_t *controller = (apps_controller_t *) userdata;
    if (node != controller->node) return;
    update_data(controller);
}

static void update_data(apps_controller_t *controller) {
    PSERVER_LIST node = controller->node;
    assert(node);
    lv_obj_t *applist = controller->applist;
    lv_obj_t *appload = controller->appload;
    if (node->state.code == SERVER_STATE_NONE) {
        lv_obj_add_flag(applist, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(appload, LV_OBJ_FLAG_HIDDEN);
    } else if (node->state.code == SERVER_STATE_ONLINE) {
        if (node->appload) {
            lv_obj_add_flag(applist, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(appload, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_clear_flag(applist, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(appload, LV_OBJ_FLAG_HIDDEN);
            lv_grid_set_data(controller->applist, node->apps);
        }
    } else {
        lv_obj_add_flag(applist, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(appload, LV_OBJ_FLAG_HIDDEN);
    }
}

static void appitem_bind(apps_controller_t *controller, lv_obj_t *item, struct _APP_DLIST *app) {
    appitem_viewholder_t *holder = lv_obj_get_user_data(item);

    coverloader_display(controller->coverloader, controller->node, app->id, item, controller->col_width,
                        controller->col_height);

    if (controller->node->server->currentGame == app->id) {
        lv_obj_clear_flag(holder->play_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(holder->close_btn, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(holder->play_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(holder->close_btn, LV_OBJ_FLAG_HIDDEN);
    }
    holder->app = app;
}


static void launcher_open_game(lv_event_t *event) {
    apps_controller_t *controller = lv_event_get_user_data(event);
    lv_obj_t *target = lv_event_get_target(event);
    if (lv_obj_get_parent(target) != lv_event_get_current_target(event)) {
        return;
    }
    appitem_viewholder_t *holder = (appitem_viewholder_t *) lv_obj_get_user_data(target);
    STREAMING_SCENE_ARGS args = {
            .server = controller->node->server,
            .app = (PAPP_DLIST) holder->app
    };
//    uimanager_push(app_uimanager, streaming_controller, &args);
}

static void launcher_resume_game(lv_event_t *event) {
    apps_controller_t *controller = lv_event_get_user_data(event);
    lv_obj_t *item = lv_obj_get_parent(lv_event_get_current_target(event));
    STREAMING_SCENE_ARGS args = {
            .server = controller->node->server,
            .app = (PAPP_DLIST) ((appitem_viewholder_t *) lv_obj_get_user_data(item))->app
    };
    uimanager_push(app_uimanager, streaming_controller, &args);
}

static void launcher_quit_game(lv_event_t *event) {
    apps_controller_t *controller = event->user_data;
    pcmanager_quitapp(controller->node->server, NULL);
}

static int adapter_item_count(lv_obj_t *adapter, void *data) {
    apps_controller_t *controller = lv_obj_get_user_data(adapter);
    int len = applist_len(data) * 7;
    // LVGL can only display up to 255 rows/columns of items
    return LV_MIN(len, 255 * controller->col_count);
}

static lv_obj_t *adapter_create_view(lv_obj_t *parent) {
    apps_controller_t *controller = lv_obj_get_user_data(parent);
    lv_obj_t *item = appitem_view(parent, &controller->appitem_style);
    // Remove from original group
    lv_group_remove_obj(item);

    appitem_viewholder_t *holder = item->user_data;
//    lv_obj_add_event_cb(holder->play_btn, launcher_resume_game, LV_EVENT_CLICKED, controller);
//    lv_obj_add_event_cb(holder->close_btn, launcher_quit_game, LV_EVENT_CLICKED, controller);
    return item;
}

static void adapter_bind_view(lv_obj_t *obj, lv_obj_t *item_view, void *data, int position) {
    apps_controller_t *controller = lv_obj_get_user_data(obj);
    appitem_bind(controller, item_view, applist_nth(data, position % applist_len(data)));
}

static int adapter_item_id(lv_obj_t *adapter, void *data, int position) {
    return applist_nth(data, position % applist_len(data))->id;
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