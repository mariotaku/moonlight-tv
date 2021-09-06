#include "app.h"
#include "launcher.controller.h"
#include "apps.controller.h"

static void launcher_view_init(lv_obj_controller_t *self, lv_obj_t *view);

static void launcher_view_destroy(lv_obj_controller_t *self, lv_obj_t *view);

static void launcher_handle_server_updated(void *userdata, PPCMANAGER_RESP resp);

static void update_pclist(launcher_controller_t *controller);

static void cb_pc_selected(lv_event_t *event);

lv_obj_controller_t *launcher_controller(void *args) {
    (void) args;
    launcher_controller_t *controller = lv_mem_alloc(sizeof(launcher_controller_t));
    lv_memset_00(controller, sizeof(launcher_controller_t));
    controller->base.create_view = launcher_win_create;
    controller->base.destroy_controller = (void (*)(struct lv_obj_controller_t *)) lv_mem_free;
    controller->base.view_created = launcher_view_init;
    controller->base.destroy_view = launcher_view_destroy;
    controller->_pcmanager_callbacks.added = launcher_handle_server_updated;
    controller->_pcmanager_callbacks.updated = launcher_handle_server_updated;
    controller->_pcmanager_callbacks.userdata = controller;
    for (PSERVER_LIST cur = computer_list; cur != NULL; cur = cur->next) {
        if (cur->selected) {
            controller->selected_server = cur;
            break;
        }
    }
    return (lv_obj_controller_t *) controller;
}

static void launcher_view_init(lv_obj_controller_t *self, lv_obj_t *view) {
    launcher_controller_t *controller = (launcher_controller_t *) self;
    pcmanager_register_callbacks(&controller->_pcmanager_callbacks);
    controller->pane_manager = uimanager_new(controller->right);
    update_pclist(controller);

    for (PSERVER_LIST cur = computer_list; cur != NULL; cur = cur->next) {
        if (!cur->selected) continue;
        uimanager_replace(controller->pane_manager, apps_controller, cur);
        break;
    }
}

static void launcher_view_destroy(lv_obj_controller_t *self, lv_obj_t *view) {
    launcher_controller_t *controller = (launcher_controller_t *) self;
    uimanager_destroy(controller->pane_manager);
    pcmanager_unregister_callbacks(&controller->_pcmanager_callbacks);
}

void launcher_handle_server_updated(void *userdata, PPCMANAGER_RESP resp) {
    launcher_controller_t *controller = userdata;
    update_pclist(controller);
}

static void cb_pc_selected(lv_event_t *event) {
    launcher_controller_t *controller = event->user_data;
    PSERVER_LIST selected = event->target->user_data;
    uimanager_replace(controller->pane_manager, apps_controller, selected);
    int pclen = lv_obj_get_child_cnt(controller->pclist);
    for (int i = 0; i < pclen; i++) {
        lv_obj_t *pcitem = lv_obj_get_child(controller->pclist, i);
        PSERVER_LIST cur = (PSERVER_LIST) lv_obj_get_user_data(pcitem);
        cur->selected = cur == selected;
        if (!cur->selected) {
            lv_obj_clear_state(pcitem, LV_STATE_CHECKED);
        } else {
            lv_obj_add_state(pcitem, LV_STATE_CHECKED);
        }
    }
}

static void update_pclist(launcher_controller_t *controller) {
    lv_obj_clean(controller->pclist);
    for (PSERVER_LIST cur = computer_list; cur != NULL; cur = cur->next) {
        lv_obj_t *pcitem = lv_list_add_btn(controller->pclist, LV_SYMBOL_DUMMY, cur->server->hostname);
        lv_obj_set_style_bg_color(pcitem, lv_palette_main(LV_PALETTE_BLUE), LV_STATE_CHECKED);
        lv_obj_set_user_data(pcitem, cur);
        lv_obj_add_event_cb(pcitem, cb_pc_selected, LV_EVENT_CLICKED, controller);
        if (!cur->selected) {
            lv_obj_clear_state(pcitem, LV_STATE_CHECKED);
        } else {
            lv_obj_add_state(pcitem, LV_STATE_CHECKED);
        }
    }
}
