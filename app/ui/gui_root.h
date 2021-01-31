#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifndef NK_NUKLEAR_H_
#include "nuklear/config.h"
#include "nuklear.h"
#include "nuklear/ext_image.h"
#include "nuklear/ext_sprites.h"
#endif

#include "util/navkey.h"

enum UI_INPUT_MODE
{
    UI_INPUT_MODE_POINTER,
    UI_INPUT_MODE_KEY,
    UI_INPUT_MODE_GAMEPAD
};
#define UI_INPUT_MODE_COUNT 3

extern short gui_display_width, gui_display_height;
extern short gui_logic_width, gui_logic_height;
extern bool gui_settings_showing;
extern enum UI_INPUT_MODE ui_input_mode;
extern bool ui_fake_mouse_click_started;

void gui_root_init(struct nk_context *ctx);

void gui_root_destroy();

bool gui_root(struct nk_context *ctx);

void gui_render_background();

bool gui_dispatch_userevent(struct nk_context *ctx, int which, void *data1, void *data2);

/**
 * @brief Check if GUI should consume input events, so it will not pass onto streaming
 */
bool gui_should_block_input();

void gui_display_size(struct nk_context *ctx, short width, short height);

bool gui_dispatch_navkey(struct nk_context *ctx, NAVKEY navkey, bool down, uint32_t timestamp);

bool ui_set_input_mode(enum UI_INPUT_MODE mode);