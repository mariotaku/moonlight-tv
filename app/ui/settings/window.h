#pragma once

#include <stdbool.h>

#include "ui/config.h"
#include "util/navkey.h"

#ifndef NK_NUKLEAR_H_
#include "nuklear/config.h"
#include "nuklear.h"
#include "nuklear/ext_functions.h"
#include "nuklear/ext_image.h"
#include "nuklear/ext_text.h"
#include "nuklear/ext_sprites.h"
#include "nuklear/ext_styling.h"
#include "nuklear/ext_imgview.h"
#include "nuklear/platform_sprites.h"
#endif

#include "stream/settings.h"
#include "app.h"

extern bool settings_pane_focused;
extern bool settings_showing_combo;
extern struct nk_vec2 settings_focused_item_center;
extern int settings_hovered_item;

void settings_window_init(struct nk_context *ctx);

bool settings_window_open();

bool settings_window_close();

bool settings_window(struct nk_context *ctx);

bool settings_window_dispatch_navkey(struct nk_context *ctx, NAVKEY navkey, NAVKEY_STATE state, uint32_t timestamp);

void settings_item_update_selected_bounds(struct nk_context *ctx, int index, struct nk_rect *bounds);

void settings_pane_item_offset(int offset);

void settings_draw_highlight(struct nk_context *ctx);