//
// Created by Mariotaku on 2021/08/31.
//

#include "app.h"

#include <malloc.h>
#include <string.h>
#include <backend/coverloader.h>
#include "lvgl/lv_ext_utils.h"
#include "backend/appmanager.h"
#include "ui/streaming/overlay.h"
#include "ui/streaming/streaming.controller.h"
#include "apps.controller.h"

typedef struct {
    ui_view_controller_t base;
    PCMANAGER_CALLBACKS _pcmanager_callbacks;
    APPMANAGER_CALLBACKS _appmanager_callbacks;
    lv_group_t *group;
    PSERVER_LIST node;
    lv_obj_t *applist, *appload;

    lv_style_t btn_style;
    int col_count;
    lv_coord_t col_width, col_height;
} apps_controller_t;

typedef struct {
    PAPP_DLIST app;
    lv_obj_t *play_btn;
    lv_obj_t *close_btn;
} appitem_viewholder_t;

static lv_obj_t *apps_view(ui_view_controller_t *self, lv_obj_t *parent);

static void on_view_created(ui_view_controller_t *self, lv_obj_t *view);

static void on_destroy_view(ui_view_controller_t *self, lv_obj_t *view);

static void on_host_updated(void *userdata, PPCMANAGER_RESP resp);

static void on_apps_updated(void *userdata, PSERVER_LIST node);

static void launcher_open_game(lv_event_t *event);

static void launcher_resume_game(lv_event_t *event);

static void launcher_quit_game(lv_event_t *event);

static void update_data(apps_controller_t *controller);

static lv_obj_t *appitem_view(apps_controller_t *controller, lv_obj_t *parent);

static void appitem_holder_free_cb(lv_event_t *event);

static void appitem_bind(apps_controller_t *controller, lv_obj_t *item, struct _APP_DLIST *app);

ui_view_controller_t *apps_controller(void *args) {
    apps_controller_t *controller = malloc(sizeof(apps_controller_t));
    memset(controller, 0, sizeof(apps_controller_t));
    controller->base.create_view = apps_view;
    controller->base.view_created = on_view_created;
    controller->base.destroy_view = on_destroy_view;
    controller->base.destroy_controller = (void (*)(ui_view_controller_t *)) free;
    controller->_pcmanager_callbacks.added = NULL;
    controller->_pcmanager_callbacks.updated = on_host_updated;
    controller->_pcmanager_callbacks.userdata = controller;
    controller->_appmanager_callbacks.updated = on_apps_updated;
    controller->_appmanager_callbacks.userdata = controller;
    controller->node = args;

    lv_style_init(&controller->btn_style);
    lv_style_set_size(&controller->btn_style, lv_dpx(40));
    lv_style_set_radius(&controller->btn_style, LV_RADIUS_CIRCLE);
    lv_style_set_bg_color(&controller->btn_style, lv_color_white());
    lv_style_set_text_color(&controller->btn_style, lv_color_black());
    return (ui_view_controller_t *) controller;
}

static lv_obj_t *apps_view(ui_view_controller_t *self, lv_obj_t *parent) {
    apps_controller_t *controller = (apps_controller_t *) self;

    lv_obj_t *applist = lv_obj_create(parent);
    lv_obj_set_style_pad_gap(applist, lv_dpx(25), 0);
    lv_obj_set_style_radius(applist, 0, 0);
    lv_obj_set_style_border_side(applist, LV_BORDER_SIDE_NONE, 0);
    lv_obj_set_style_bg_opa(applist, 0, 0);
    lv_obj_set_size(applist, LV_PCT(100), LV_PCT(100));
    lv_obj_set_layout(applist, LV_LAYOUT_GRID);
    lv_obj_t *appload = lv_spinner_create(parent, 1000, 60);
    lv_obj_set_size(appload, lv_dpx(60), lv_dpx(60));
    lv_obj_center(appload);

    controller->applist = applist;
    controller->appload = appload;
    return NULL;
}

static void on_view_created(ui_view_controller_t *self, lv_obj_t *view) {
    apps_controller_t *controller = (apps_controller_t *) self;
    appmanager_register_callbacks(&controller->_appmanager_callbacks);
    pcmanager_register_callbacks(&controller->_pcmanager_callbacks);
    lv_obj_t *applist = controller->applist;
    lv_group_remove_obj(applist);
    controller->group = lv_group_create();
    lv_indev_set_group(app_indev_key, controller->group);

    update_data(controller);
    if (!controller->node->apps) {
        application_manager_load(controller->node);
    }

    int col_count = 5;
    lv_coord_t applist_width = lv_measure_width(applist);
    lv_coord_t col_width = (applist_width - lv_obj_get_style_pad_left(applist, 0) -
                            lv_obj_get_style_pad_right(applist, 0) -
                            lv_obj_get_style_pad_column(applist, 0) * (col_count - 1)) / col_count;
    controller->col_count = col_count;
    controller->col_width = col_width;
    controller->col_height = col_width / 3 * 4;
}

static void on_destroy_view(ui_view_controller_t *self, lv_obj_t *view) {
    apps_controller_t *controller = (apps_controller_t *) self;
    lv_group_del(controller->group);
    appmanager_unregister_callbacks(&controller->_appmanager_callbacks);
    pcmanager_unregister_callbacks(&controller->_pcmanager_callbacks);

    lv_indev_set_group(app_indev_key, lv_group_get_default());
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
            if (lv_obj_get_child_cnt(applist) == 0) {
                int len = applist_len(node->apps);
                int col_count = controller->col_count;
                int row_count = len / col_count + 1;
                lv_coord_t *row_dsc = malloc((row_count + 1) * sizeof(lv_coord_t));
                lv_coord_t *col_dsc = malloc((col_count + 1) * sizeof(lv_coord_t));
                for (int i = 0; i < row_count; i++) {
                    row_dsc[i] = controller->col_height;
                }
                row_dsc[row_count] = LV_GRID_TEMPLATE_LAST;
                for (int i = 0; i < col_count; i++) {
                    col_dsc[i] = LV_GRID_FR(1);
                }
                col_dsc[col_count] = LV_GRID_TEMPLATE_LAST;
                lv_obj_set_grid_dsc_array(applist, col_dsc, row_dsc);
                lv_coord_t col_pos = 0, row_pos = 0;
                for (PAPP_DLIST cur = node->apps; cur != NULL; cur = cur->next) {
                    lv_obj_t *item = appitem_view(controller, applist);

                    appitem_bind(controller, item, cur);

                    item->user_data = cur;

                    lv_group_remove_obj(item);
                    lv_group_add_obj(controller->group, item);

                    lv_obj_set_grid_cell(item, LV_GRID_ALIGN_STRETCH, col_pos, 1, LV_GRID_ALIGN_STRETCH, row_pos, 1);
                    col_pos++;
                    if (col_pos >= col_count) {
                        col_pos = 0;
                        row_pos++;
                    }
                }
            }
        }
    } else {
        lv_obj_add_flag(applist, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(appload, LV_OBJ_FLAG_HIDDEN);
    }
}

static lv_obj_t *appitem_view(apps_controller_t *controller, lv_obj_t *parent) {
    lv_obj_t *item = lv_btn_create(parent);
    lv_obj_set_style_pad_all(item, 0, 0);
    lv_obj_set_style_radius(item, 0, 0);
    lv_obj_set_style_shadow_opa(item, LV_OPA_40, 0);
    lv_obj_set_style_shadow_width(item, lv_dpx(15), 0);
    lv_obj_set_style_shadow_ofs_y(item, lv_dpx(3), 0);

    lv_obj_t *play_btn = lv_btn_create(item);
    lv_obj_set_style_bg_img_src(play_btn, LV_SYMBOL_PLAY, 0);
    lv_obj_add_style(play_btn, &controller->btn_style, 0);
    lv_obj_align(play_btn, LV_ALIGN_CENTER, 0, -lv_dpx(25));
    lv_obj_t *close_btn = lv_btn_create(item);
    lv_obj_set_style_bg_img_src(close_btn, LV_SYMBOL_CLOSE, 0);
    lv_obj_add_style(close_btn, &controller->btn_style, 0);
    lv_obj_align(close_btn, LV_ALIGN_CENTER, 0, lv_dpx(25));

    appitem_viewholder_t *holder = (appitem_viewholder_t *) malloc(sizeof(appitem_viewholder_t));
    memset(holder, 0, sizeof(appitem_viewholder_t));
    holder->play_btn = play_btn;
    holder->close_btn = close_btn;
    item->user_data = holder;
    lv_obj_add_event_cb(item, appitem_holder_free_cb, LV_EVENT_DELETE, NULL);
    lv_obj_add_event_cb(item, launcher_open_game, LV_EVENT_CLICKED, controller);
    lv_obj_add_event_cb(play_btn, launcher_open_game, LV_EVENT_CLICKED, controller);
    lv_obj_add_event_cb(close_btn, launcher_quit_game, LV_EVENT_CLICKED, controller);
    return item;
}

static void appitem_bind(apps_controller_t *controller, lv_obj_t *item, struct _APP_DLIST *app) {
    appitem_viewholder_t *holder = item->user_data;

    coverloader_display(controller->node, app->id, item, controller->col_width, controller->col_height);

    if (controller->node->server->currentGame == app->id) {
        lv_obj_clear_flag(holder->play_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(holder->close_btn, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(holder->play_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(holder->close_btn, LV_OBJ_FLAG_HIDDEN);
    }
    holder->app = app;
}

static void appitem_holder_free_cb(lv_event_t *event) {
    appitem_viewholder_t *holder = event->target->user_data;
    free(holder);
}

static void launcher_open_game(lv_event_t *event) {
    apps_controller_t *controller = event->user_data;
    STREAMING_SCENE_ARGS args = {
            .server = controller->node->server,
            .app = (PAPP_DLIST) ((appitem_viewholder_t *) event->target->user_data)->app
    };
    uimanager_push(app_uimanager, streaming_controller, &args);
}

static void launcher_resume_game(lv_event_t *event) {
    apps_controller_t *controller = event->user_data;
    STREAMING_SCENE_ARGS args = {
            .server = controller->node->server,
            .app = (PAPP_DLIST) ((appitem_viewholder_t *) event->target->user_data)->app
    };
    uimanager_push(app_uimanager, streaming_controller, &args);
}

static void launcher_quit_game(lv_event_t *event) {
    apps_controller_t *controller = event->user_data;
    pcmanager_quitapp(controller->node->server, NULL);
}
