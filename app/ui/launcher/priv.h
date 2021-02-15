#pragma once

#include <stdbool.h>

#include "backend/computer_manager.h"

#include "libgamestream/errors.h"

typedef enum pairing_state
{
    PS_NONE,
    PS_RUNNING,
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
#define LAUNCHER_MODAL_MANPAIR 0x0004
#define LAUNCHER_MODAL_MASK_POPUP 0xFF00
#define LAUNCHER_MODAL_SERVERR 0x0100
#define LAUNCHER_MODAL_PAIRERR 0x0200
#define LAUNCHER_MODAL_QUITERR 0x0300
#define LAUNCHER_MODAL_WDECERR 0x0400

extern uint32_t _launcher_modals;
extern bool _launcher_popup_request_dismiss;
extern struct pairing_computer_state pairing_computer_state;
extern bool _webos_decoder_error_dismissed;
extern bool _quitapp_errno;
extern bool _launcher_show_manual_pair;

void _select_computer(PSERVER_LIST node, bool load_apps);

void _open_pair(PSERVER_LIST node);
