#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "ui/config.h"

#include "util/navkey.h"

enum UI_INPUT_MODE {
    UI_INPUT_MODE_POINTER,
    UI_INPUT_MODE_KEY,
    UI_INPUT_MODE_REMOTE,
    UI_INPUT_MODE_GAMEPAD
};
#define UI_INPUT_MODE_COUNT 3

#define UI_BOTTOM_BAR_HEIGHT_DP 20
#define UI_TITLE_BAR_HEIGHT_DP 25

extern short ui_display_width, ui_display_height;
extern short ui_logic_width, ui_logic_height;
extern enum UI_INPUT_MODE ui_input_mode;

bool ui_dispatch_userevent(int which, void *data1, void *data2);

/**
 * @brief Check if GUI should consume input events, so it will not pass onto streaming
 */
bool ui_should_block_input();

void ui_display_size(short width, short height);

bool ui_set_input_mode(enum UI_INPUT_MODE mode);
