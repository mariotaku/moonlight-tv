#pragma once

#include <stdbool.h>

#include <SDL.h>

#ifndef NK_NUKLEAR_H_
#include "nuklear/config.h"
#include "nuklear.h"
#endif

extern short gui_display_width, gui_display_height;
extern bool gui_settings_opened;

void gui_root_init(struct nk_context *ctx);

bool gui_root(struct nk_context *ctx);

void gui_background();

bool gui_dispatch_userevent(struct nk_context *ctx, SDL_Event ev);

bool gui_dispatch_inputevent(struct nk_context *ctx, SDL_Event ev);

void gui_display_size(short width, short height);