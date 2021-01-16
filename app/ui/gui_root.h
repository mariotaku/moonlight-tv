#pragma once

#include <stdbool.h>

#ifdef HAVE_SDL
#include <SDL_events.h>
#endif

#ifndef NK_NUKLEAR_H_
#include "nuklear/config.h"
#include "nuklear.h"
#include "nuklear/ext_image.h"
#endif

extern short gui_display_width, gui_display_height;
extern short gui_logic_width, gui_logic_height;
extern bool gui_settings_showing;

void gui_root_init(struct nk_context *ctx);

void gui_root_destroy();

bool gui_root(struct nk_context *ctx);

void gui_background();

bool gui_dispatch_userevent(int which, void *data1, void *data2);

/**
 * @brief Check if GUI should consume input events, so it will not pass onto streaming
 */
bool gui_should_block_input();

void gui_display_size(struct nk_context *ctx, short width, short height);

#ifdef HAVE_SDL
bool gui_dispatch_inputevent(struct nk_context *ctx, SDL_Event ev);
#endif