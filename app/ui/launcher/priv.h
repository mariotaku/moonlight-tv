#pragma once

#include <stdbool.h>

#include "backend/computer_manager.h"

#include "libgamestream/errors.h"

#include "lvgl.h"

typedef enum pairing_state
{
    PS_NONE,
    PS_PAIRING,
    PS_UNPAIRING,
    PS_FAIL
} pairing_state;

struct pairing_computer_state
{
    pairing_state state;
    char pin[5];
    const char *error;
};

#define LAUNCHER_MODAL_MASK_WINDOW 0x00FF
#define LAUNCHER_MODAL_PAIRING 0x0001
#define LAUNCHER_MODAL_QUITAPP 0x0002
#define LAUNCHER_MODAL_MANUAL_ADD 0x0004
#define LAUNCHER_MODAL_UNPAIRING 0x0008
#define LAUNCHER_MODAL_CERTGEN 0x0010
#define LAUNCHER_MODAL_MASK_POPUP 0xFF00
#define LAUNCHER_MODAL_SERVERR 0x0100
#define LAUNCHER_MODAL_PAIRERR 0x0200
#define LAUNCHER_MODAL_QUITERR 0x0400
#define LAUNCHER_MODAL_HOSTINFO 0x0800
#define LAUNCHER_MODAL_NOCODEC 0x1000
#define LAUNCHER_MODAL_NOHWCODEC 0x2000

extern uint32_t _launcher_modals;
extern bool _launcher_popup_request_dismiss;
extern struct pairing_computer_state pairing_computer_state;
extern bool _decoder_error_dismissed;
extern bool _quitapp_errno;
extern bool _launcher_show_manual_pair;
extern bool _launcher_show_host_info;
extern bool computer_manager_executing_quitapp;
extern struct nk_vec2 _computer_picker_center;

void _select_computer(PSERVER_LIST node, bool load_apps);

void _open_pair(PSERVER_LIST node);
void _open_unpair(PSERVER_LIST node);

void launcher_handle_quitapp(PPCMANAGER_RESP resp);
void handle_unpairing_done(PPCMANAGER_RESP resp);

void launcher_controller_init();
void launcher_controller_pc_selected(lv_event_t *event);

PSERVER_LIST launcher_win_selected_server();
void launcher_win_update_pclist();
void launcher_win_update_selected();