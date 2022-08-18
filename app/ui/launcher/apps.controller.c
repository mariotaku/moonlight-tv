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

#include "lvgl/lv_ext_utils.h"
#include "lvgl/ext/lv_gridview.h"
#include "lvgl/util/lv_app_utils.h"

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

static void host_info_cb(const pcmanager_resp_t *resp, void *userdata);

static void send_wol_cb(const pcmanager_resp_t *resp, void *userdata);

static void on_host_updated(const pcmanager_resp_t *resp, void *userdata);

static void on_host_removed(const pcmanager_resp_t *resp, void *userdata);

static void item_click_cb(lv_event_t *event);

static void item_longpress_cb(lv_event_t *event);

static void launcher_launch_game(apps_fragment_t *controller, const apploader_item_t *app);

static void launcher_toggle_fav(apps_fragment_t *controller, const apploader_item_t *app);

static void launcher_quit_game(apps_fragment_t *controller);

static void applist_focus_enter(lv_event_t *event);

static void applist_focus_leave(lv_event_t *event);

static void update_view_state(apps_fragment_t *controller);

static void appitem_bind(apps_fragment_t *controller, lv_obj_t *item, apploader_item_t *app);

static int adapter_item_count(lv_obj_t *, void *data);

static lv_obj_t *adapter_create_view(lv_obj_t *parent);

static void adapter_bind_view(lv_obj_t *, lv_obj_t *, void *data, int position);

static int adapter_item_id(lv_obj_t *, void *data, int position);

static void quitgame_cb(const pcmanager_resp_t *resp, void *userdata);

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

static void open_context_menu(apps_fragment_t *fragment, apploader_item_t *app);

static void context_menu_cancel_cb(lv_event_t *event);

static void context_menu_click_cb(lv_event_t *event);

static void app_detail_dialog(apps_fragment_t *controller, apploader_item_t *app);

static void app_detail_click_cb(lv_event_t *event);

static void set_actions(apps_fragment_t *controller, const char **labels, const action_cb_t *callbacks);

static apps_fragment_t *current_instance = NULL;

const static lv_gridview_adapter_t apps_adapter = {
        .item_count = adapter_item_count,
        .create_view = adapter_create_view,
        .bind_view = adapter_bind_view,
        .item_id = adapter_item_id,
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
    controller->uuid = *(const uuidstr_t *) args;
    controller->apploader = apploader_create(&controller->uuid, &controller->apploader_cb, controller);

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

    lv_obj_t *applist = controller->applist = lv_gridview_create(view);
    lv_obj_add_flag(applist, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_style_pad_all(applist, lv_dpx(24), 0);
    lv_obj_set_style_pad_gap(applist, lv_dpx(24), 0);
    lv_obj_set_style_radius(applist, 0, 0);
    lv_obj_set_style_border_side(applist, LV_BORDER_SIDE_NONE, 0);
    lv_obj_set_style_bg_opa(applist, 0, 0);
    lv_obj_set_size(applist, LV_PCT(100), LV_PCT(100));
    lv_gridview_set_adapter(applist, &apps_adapter);
    lv_obj_t *appload = controller->appload = lv_spinner_create(view, 1000, 60);
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
    apps_fragment_t *controller = (apps_fragment_t *) self;
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

    const SERVER_STATE *state = pcmanager_state(pcmanager, controller->uuid);
    if (state->code != SERVER_STATE_QUERYING) {
        pcmanager_request_update(pcmanager, controller->uuid, host_info_cb, controller);
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
    current_instance = NULL;
    apps_fragment_t *controller = (apps_fragment_t *) self;

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
        default:
            break;
    }
    return false;
}

static void on_host_updated(const pcmanager_resp_t *resp, void *userdata) {
    apps_fragment_t *controller = (apps_fragment_t *) userdata;
    if (controller != current_instance) return;
    if (!uuidstr_equals(resp->server->uuid, controller->uuid)) return;
    apploader_load(controller->apploader);
    update_view_state(controller);
}

static void on_host_removed(const pcmanager_resp_t *resp, void *userdata) {
    apps_fragment_t *controller = (apps_fragment_t *) userdata;
    if (!uuidstr_equals(resp->server->uuid, controller->uuid)) return;
    lv_fragment_del((lv_fragment_t *) controller);
}

static void host_info_cb(const pcmanager_resp_t *resp, void *userdata) {
    apps_fragment_t *controller = (apps_fragment_t *) userdata;
    if (controller != current_instance) return;
    if (!controller->base.managed->obj_created) return;
    lv_btnmatrix_clear_btn_ctrl_all(controller->actions, LV_BTNMATRIX_CTRL_DISABLED);
    if (resp->state.code == SERVER_STATE_AVAILABLE && !controller->apploader_apps) {
        apploader_load(controller->apploader);
    }
}

static void send_wol_cb(const pcmanager_resp_t *resp, void *userdata) {
    apps_fragment_t *controller = (apps_fragment_t *) userdata;
    if (controller != current_instance) return;
    if (!controller->base.managed->obj_created) return;
    lv_btnmatrix_clear_btn_ctrl_all(controller->actions, LV_BTNMATRIX_CTRL_DISABLED);
    const SERVER_STATE *state = pcmanager_state(pcmanager, controller->uuid);
    SDL_assert(state != NULL);
    if (state->code & SERVER_STATE_ONLINE || resp->result.code != GS_OK) return;
    pcmanager_request_update(pcmanager, controller->uuid, host_info_cb, controller);
}

static void update_view_state(apps_fragment_t *controller) {
    if (controller != current_instance) return;
    if (!controller->base.managed->obj_created || controller->base.managed->destroying_obj) return;
    launcher_controller_t *parent_controller = (launcher_controller_t *) lv_fragment_get_parent(&controller->base);
    parent_controller->detail_changing = true;
    lv_obj_t *applist = controller->applist;
    lv_obj_t *appload = controller->appload;
    lv_obj_t *apperror = controller->apperror;
    const SERVER_STATE *state = pcmanager_state(pcmanager, controller->uuid);
    SDL_assert(state != NULL);
    switch (state->code) {
        case SERVER_STATE_NONE:
        case SERVER_STATE_QUERYING: {
            // waiting to load server info
            lv_obj_add_flag(applist, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(apperror, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(appload, LV_OBJ_FLAG_HIDDEN);
            break;
        }
        case SERVER_STATE_AVAILABLE: {
            switch (apploader_state(controller->apploader)) {
                case APPLOADER_STATE_LOADING: {
                    // is loading apps
                    if (controller->apploader_apps) {
                        break;
                    }
                    lv_obj_add_flag(applist, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_add_flag(apperror, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_clear_flag(appload, LV_OBJ_FLAG_HIDDEN);
                    break;
                }
                case APPLOADER_STATE_ERROR: {
                    // apploader has error
                    lv_obj_add_flag(appload, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_clear_flag(apperror, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_add_flag(applist, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_set_style_opa(controller->errordetail, LV_OPA_0, 0);
                    lv_obj_clear_flag(controller->actions, LV_OBJ_FLAG_HIDDEN);

                    lv_label_set_text_static(controller->errortitle, locstr("Failed to load apps"));
                    lv_label_set_text_static(controller->errorhint, locstr(
                            "Press \"Retry\" to load again.\n\nRestart your computer if error persists."));
                    lv_label_set_text_static(controller->errordetail, controller->apploader_error);
                    set_actions(controller, action_labels_error, action_callbacks_error);

                    lv_group_focus_obj(controller->actions);
                    lv_obj_add_state(controller->actions, LV_STATE_FOCUS_KEY);
                    break;
                }
                case APPLOADER_STATE_IDLE: {
                    // has apps
                    lv_obj_clear_flag(applist, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_add_flag(apperror, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_add_flag(appload, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_add_flag(controller->actions, LV_OBJ_FLAG_HIDDEN);

                    if (lv_group_get_focused(lv_obj_get_group(controller->applist)) != controller->applist) {
                        lv_group_focus_obj(controller->applist);
                    }
                    break;
                }
            }
            break;
        }
        case SERVER_STATE_NOT_PAIRED: {
            lv_obj_add_flag(appload, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(apperror, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(applist, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_opa(controller->errordetail, LV_OPA_0, 0);
            lv_obj_clear_flag(controller->actions, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text_static(controller->errortitle, locstr("Not paired"));
            lv_label_set_text_static(controller->errorhint, locstr(
                    "Press \"Pair\", and input PIN code on your computer to pair."));
            set_actions(controller, actions_unpaired, action_callbacks_unpaired);

            lv_group_focus_obj(controller->actions);
            lv_obj_add_state(controller->actions, LV_STATE_FOCUS_KEY);
            break;
        }
        case SERVER_STATE_ERROR: {
            // server has error
            lv_obj_add_flag(appload, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(apperror, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(applist, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_opa(controller->errordetail, LV_OPA_100, 0);
            lv_obj_clear_flag(controller->actions, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text_static(controller->errortitle, locstr("Host error"));
            lv_label_set_text_static(controller->errorhint, locstr(
                    "Press \"Retry\" to load again.\n\nRestart your computer if error persists."));
            lv_label_set_text_static(controller->errordetail, state->error.errmsg);
            set_actions(controller, action_labels_error, action_callbacks_error);

            lv_group_focus_obj(controller->actions);
            lv_obj_add_state(controller->actions, LV_STATE_FOCUS_KEY);
            break;
        }
        case SERVER_STATE_OFFLINE: {
            // server has error
            lv_obj_add_flag(appload, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(apperror, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(applist, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_opa(controller->errordetail, LV_OPA_0, 0);
            lv_obj_clear_flag(controller->actions, LV_OBJ_FLAG_HIDDEN);
            lv_btnmatrix_clear_btn_ctrl(controller->actions, 0, LV_BTNMATRIX_CTRL_DISABLED);
            lv_label_set_text_static(controller->errortitle, locstr("Offline"));
            lv_label_set_text_static(controller->errorhint, locstr(
                    "Press \"Wake\" to send Wake-on-LAN packet to turn the computer on if it supports this feature, "
                    "press \"Retry\" to connect again.\n\n"
                    "Try restart your computer if these doesn't work."
            ));
            set_actions(controller, action_labels_offline, action_callbacks_offline);

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


static void appload_started(void *userdata) {
    apps_fragment_t *fragment = userdata;
    update_view_state(fragment);
}

static void appload_loaded(apploader_list_t *apps, void *userdata) {
    apps_fragment_t *fragment = userdata;
    lv_gridview_set_data(fragment->applist, apps);
    apploader_list_free(fragment->apploader_apps);
    fragment->apploader_apps = apps;
    update_view_state(fragment);
}

static void appload_errored(int code, const char *error, void *userdata) {
    LV_UNUSED(code);
    apps_fragment_t *fragment = userdata;
    fragment->apploader_error = error;
    update_view_state(fragment);
}

static void appitem_bind(apps_fragment_t *controller, lv_obj_t *item, apploader_item_t *app) {
    appitem_viewholder_t *holder = lv_obj_get_user_data(item);

    coverloader_display(controller->coverloader, controller->uuid, app->base.id, item,
                        controller->col_width, controller->col_height);
    lv_label_set_text(holder->title, app->base.name);

    int appid = pcmanager_server_current_app(pcmanager, controller->uuid);
    if (appid == app->base.id) {
        lv_obj_clear_flag(holder->play_indicator, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(holder->play_indicator, LV_OBJ_FLAG_HIDDEN);
    }
    holder->app = app;
}

static void item_click_cb(lv_event_t *event) {
    apps_fragment_t *controller = lv_event_get_user_data(event);
    lv_obj_t *target = lv_event_get_target(event);
    lv_obj_t *target_parent = lv_obj_get_parent(target);
    if (target_parent != controller->applist) {
        return;
    }
    appitem_viewholder_t *holder = (appitem_viewholder_t *) lv_obj_get_user_data(target);
    int appid = pcmanager_server_current_app(pcmanager, controller->uuid);
    if (appid != 0) {
        if (holder->app->base.id == appid) {
            open_context_menu(controller, holder->app);
        }
        return;
    }
    launcher_launch_game(holder->controller, holder->app);
}

static void item_longpress_cb(lv_event_t *event) {
    apps_fragment_t *controller = lv_event_get_user_data(event);
    lv_obj_t *target = lv_event_get_target(event);
    lv_obj_t *target_parent = lv_obj_get_parent(target);
    if (target_parent != controller->applist) {
        return;
    }
    lv_event_send(target, LV_EVENT_RELEASED, lv_event_get_indev(event));
    appitem_viewholder_t *holder = (appitem_viewholder_t *) lv_obj_get_user_data(target);
    open_context_menu(controller, holder->app);
}

static void launcher_launch_game(apps_fragment_t *controller, const apploader_item_t *app) {
    streaming_scene_arg_t args = {
            .server = controller->node->server,
            .app = &app->base,
    };
    LV_ASSERT(app->base.id != 0);
    lv_fragment_t *fragment = lv_fragment_create(&streaming_controller_class, &args);
    lv_obj_t *const *container = lv_fragment_get_container(lv_fragment_manager_get_top(app_uimanager));
    lv_fragment_manager_push(app_uimanager, fragment, container);
}

static void launcher_toggle_fav(apps_fragment_t *controller, const apploader_item_t *app) {
    pcmanager_favorite_app(pcmanager, controller->uuid, app->base.id, !app->fav);
    apploader_load(controller->apploader);
}

static void launcher_quit_game(apps_fragment_t *controller) {
    controller->quit_progress = progress_dialog_create(locstr("Quitting game..."));
    pcmanager_quitapp(pcmanager, controller->uuid, quitgame_cb, controller);
}

static int adapter_item_count(lv_obj_t *grid, void *data) {
    if (data == NULL) return 0;
    apps_fragment_t *controller = lv_obj_get_user_data(grid);
    apploader_list_t *list = data;
    // LVGL can only display up to 255 rows/columns, but I don't think anyone has library that big (1275 items)
    return LV_MIN(list->count, 255 * controller->col_count);
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

static int adapter_item_id(lv_obj_t *grid, void *data, int position) {
    LV_UNUSED(grid);
    apploader_list_t *list = data;
    return list->items[position].base.id;
}


static void applist_focus_enter(lv_event_t *event) {
    if (event->target != event->current_target) return;
    apps_fragment_t *controller = lv_event_get_user_data(event);
    lv_gridview_focus(controller->applist, controller->focus_backup);
}

static void applist_focus_leave(lv_event_t *event) {
    if (event->target != event->current_target) return;
    apps_fragment_t *controller = lv_event_get_user_data(event);
    controller->focus_backup = lv_gridview_get_focused_index(controller->applist);
    lv_gridview_focus(controller->applist, -1);
}

static void quitgame_cb(const pcmanager_resp_t *resp, void *userdata) {
    apps_fragment_t *controller = userdata;
    if (controller->quit_progress) {
        lv_msgbox_close(controller->quit_progress);
        controller->quit_progress = NULL;
    }
    lv_gridview_rebind(controller->applist);
    if (resp->result.code == GS_OK) {
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
    pcmanager_send_wol(pcmanager, controller->uuid, send_wol_cb, controller);
}

static void action_cb_host_reload(apps_fragment_t *controller, lv_obj_t *buttons, uint16_t index) {
    LV_UNUSED(index);
    lv_btnmatrix_set_btn_ctrl_all(buttons, LV_BTNMATRIX_CTRL_DISABLED);
    pcmanager_request_update(pcmanager, controller->uuid, host_info_cb, controller);
}

static void action_cb_pair(apps_fragment_t *controller, lv_obj_t *buttons, uint16_t index) {
    LV_UNUSED(buttons);
    LV_UNUSED(index);
    pair_dialog_open(controller->node);
}

static void open_context_menu(apps_fragment_t *fragment, apploader_item_t *app) {
    lv_obj_t *msgbox = lv_msgbox_create(NULL, app->base.name, NULL, NULL, false);
    lv_obj_set_user_data(msgbox, app);
    lv_obj_t *content = lv_msgbox_get_content(msgbox);
    lv_obj_add_flag(content, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);

    lv_obj_add_event_cb(content, context_menu_cancel_cb, LV_EVENT_CANCEL, fragment);
    lv_obj_add_event_cb(content, context_menu_click_cb, LV_EVENT_SHORT_CLICKED, fragment);

    int currentId = fragment->node->server->currentGame;

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

    lv_obj_t *info_btn = lv_list_add_btn(content, NULL, locstr("Info"));
    lv_obj_add_flag(info_btn, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_user_data(info_btn, app_detail_dialog);

    lv_obj_t *cancel_btn = lv_list_add_btn(content, NULL, locstr("Cancel"));
    lv_obj_add_flag(cancel_btn, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_center(msgbox);
}

static void context_menu_cancel_cb(lv_event_t *e) {
    lv_obj_t *target = lv_event_get_target(e);
    if (target->parent != lv_event_get_current_target(e)) return;
    lv_msgbox_close(lv_event_get_current_target(e)->parent);
}

static void context_menu_click_cb(lv_event_t *e) {
    lv_obj_t *target = lv_event_get_target(e);
    lv_obj_t *current_target = lv_event_get_current_target(e);
    if (target->parent != current_target) return;
    lv_obj_t *mbox = lv_event_get_current_target(e)->parent;
    apps_fragment_t *controller = lv_event_get_user_data(e);
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

static void app_detail_dialog(apps_fragment_t *fragment, apploader_item_t *app) {
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
