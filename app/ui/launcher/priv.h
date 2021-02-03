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

#define LAUNCHER_MODAL_MASK_WINDOW 0b00001111
#define LAUNCHER_MODAL_PAIRING 0b00000001
#define LAUNCHER_MODAL_QUITAPP 0b00000010
#define LAUNCHER_MODAL_MANPAIR 0b00000100
#define LAUNCHER_MODAL_MASK_POPUP 0b11110000
#define LAUNCHER_MODAL_SERVERR 0b00010000
#define LAUNCHER_MODAL_PAIRERR 0b00100000
#define LAUNCHER_MODAL_QUITERR 0b01000000
#define LAUNCHER_MODAL_WDECERR 0b10000000

extern uint32_t _launcher_modals;
extern bool _launcher_popup_request_dismiss;
extern struct pairing_computer_state pairing_computer_state;
extern bool _webos_decoder_error_dismissed;
extern bool _quitapp_errno;

void _select_computer(PSERVER_LIST node, bool load_apps);

void _open_pair(int index, PSERVER_LIST node);
