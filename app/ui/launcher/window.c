#include "window.h"
#include "priv.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <memory.h>

#include "app.h"
#include "res.h"

#include "libgamestream/errors.h"

#include "backend/application_manager.h"
#include "ui/root.h"
#include "ui/settings/settings.controller.h"

#include "stream/input/absinput.h"
#include "stream/platform.h"
#include "util/bus.h"
#include "util/user_event.h"
#include "util/logging.h"

PSERVER_LIST selected_server_node;

struct nk_image launcher_default_cover;

struct pairing_computer_state pairing_computer_state;

static struct nk_style_button cm_list_button_style;

int topbar_item_hover_request = -1;
int topbar_hovered_item = -1;
struct nk_rect topbar_hovering_item_bounds = {0, 0, 0, 0};
struct nk_vec2 topbar_focused_item_center = {0, 0};
static int topbar_item_count = 0;
bool topbar_showing_combo = false;
bool computer_manager_executing_quitapp = false;

void _pairing_error_popup(struct nk_context *ctx);

void _server_error_popup(struct nk_context *ctx);

void _quitapp_window(struct nk_context *ctx);

void _quitapp_error_popup(struct nk_context *ctx);

void _decoder_warning_popup(struct nk_context *ctx);

bool pclist_dropdown(struct nk_context *ctx, bool event_emitted);

void launcher_statbar(struct nk_context *ctx);

static void _launcher_modal_flags_update();

void launcher_item_update_selected_bounds(struct nk_context *ctx, int index, struct nk_rect *bounds);

void topbar_item_offset(int offset);

#define launcher_blocked() ((_launcher_modals & LAUNCHER_MODAL_MASK_WINDOW) || ui_settings_showing)

uint32_t _launcher_modals;
bool _launcher_popup_request_dismiss;
bool _launcher_show_manual_pair;
bool _launcher_show_host_info;
bool _quitapp_errno = false;

void _select_computer(PSERVER_LIST node, bool load_apps) {
    selected_server_node = node;
    pairing_computer_state.state = PS_NONE;
    if (node->server) {
        app_configuration->address = strdup(node->server->serverInfo.address);
    }
    if (load_apps) {
        application_manager_load(node);
    }
}

void handle_pairing_done(PPCMANAGER_RESP resp) {
    if (resp->result.code == GS_OK) {
        // Close pairing window
        pairing_computer_state.state = PS_NONE;
        PSERVER_LIST node = serverlist_find_by(computer_list, resp->server->uuid, serverlist_compare_uuid);
        if (!node)
            return;
        _select_computer(node, node->apps == NULL);
    } else {
        // Show pairing error instead
        pairing_computer_state.state = PS_FAIL;
        pairing_computer_state.error = resp->result.error.message;
    }
}

void handle_unpairing_done(PPCMANAGER_RESP resp) {
    if (resp->result.code == GS_OK) {
        // Close pairing window
        pairing_computer_state.state = PS_NONE;
        selected_server_node = NULL;
    } else {
        // Show pairing error instead
        pairing_computer_state.state = PS_FAIL;
        pairing_computer_state.error = resp->result.error.message;
    }
}

void launcher_handle_server_updated(PPCMANAGER_RESP resp) {
    if (resp->result.code != GS_OK)
        return;
    PSERVER_LIST node = serverlist_find_by(computer_list, resp->server->uuid, serverlist_compare_uuid);
    if (!node)
        return;
    // Select saved paired server if not selected before
    if (resp->server && resp->server->paired && selected_server_node == node && !node->apps) {
        application_manager_load(node);
    }
}

void _open_pair(PSERVER_LIST node) {
    selected_server_node = NULL;
    pairing_computer_state.state = PS_PAIRING;
    pcmanager_pair(node->server, &pairing_computer_state.pin[0], handle_pairing_done);
}

void _open_unpair(PSERVER_LIST node) {
    if (pcmanager_unpair(node->server, handle_unpairing_done)) {
        pairing_computer_state.state = PS_UNPAIRING;
    }
}

void _launcher_modal_flags_update() {
    _launcher_modals = 0;
    if (pairing_computer_state.state != PS_NONE) {
        switch (pairing_computer_state.state) {
            case PS_PAIRING:
                _launcher_modals |= LAUNCHER_MODAL_PAIRING;
                break;
            case PS_UNPAIRING:
                _launcher_modals |= LAUNCHER_MODAL_UNPAIRING;
                break;
            case PS_FAIL:
                _launcher_modals |= LAUNCHER_MODAL_PAIRERR;
                break;
            default:
                break;
        }
    }
    if (pcmanager_setup_running) {
        _launcher_modals |= LAUNCHER_MODAL_CERTGEN;
    }
    if (computer_manager_executing_quitapp) {
        _launcher_modals |= LAUNCHER_MODAL_QUITAPP;
    }
    if (_quitapp_errno) {
        _launcher_modals |= LAUNCHER_MODAL_QUITERR;
    }
    if (_launcher_show_manual_pair) {
        _launcher_modals |= LAUNCHER_MODAL_MANUAL_ADD;
    }
    if (!_decoder_error_dismissed) {
        if (!decoder_info.valid) {
#ifndef TARGET_DESKTOP
            _launcher_modals |= LAUNCHER_MODAL_NOCODEC;
#endif
        } else if (!decoder_info.accelerated) {
#ifndef TARGET_DESKTOP
            _launcher_modals |= LAUNCHER_MODAL_NOHWCODEC;
#endif
        }
    }
    if (_launcher_show_host_info) {
        _launcher_modals |= LAUNCHER_MODAL_HOSTINFO;
    }
}

void launcher_item_update_selected_bounds(struct nk_context *ctx, int index, struct nk_rect *bounds) {
    if (!topbar_showing_combo && nk_widget_is_hovered(ctx)) {
        topbar_hovered_item = index;
        topbar_hovering_item_bounds = nk_widget_bounds(ctx);
        if (ui_input_mode == UI_INPUT_MODE_POINTER) {
            topbar_focused_item_center = nk_rect_center(topbar_hovering_item_bounds);
        }
    }
    if (topbar_item_hover_request == index) {
        *bounds = nk_widget_bounds(ctx);
        topbar_focused_item_center = nk_rect_center(*bounds);
    }
}

void topbar_item_offset(int offset) {
    int new_index = topbar_hovered_item + offset;
    if (new_index < 0) {
        topbar_item_hover_request = 0;
    } else if (new_index >= topbar_item_count) {
        topbar_item_hover_request = topbar_item_count - 1;
    } else {
        topbar_item_hover_request = new_index;
    }
}
