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

typedef bool (*settings_panel_render)(struct nk_context *, bool *showing_combo);
typedef bool (*settings_panel_navkey)(struct nk_context *, NAVKEY navkey, NAVKEY_STATE state, uint32_t timestamp);
typedef int (*settings_panel_itemcount)();
typedef void (*settings_panel_onselect)();

extern struct settings_pane settings_pane_basic;
extern struct settings_pane settings_pane_host;
extern struct settings_pane settings_pane_mouse;
extern struct settings_pane settings_pane_sysinfo;

struct settings_pane
{
    const char *title;
    settings_panel_render render;
    settings_panel_navkey navkey;
    settings_panel_itemcount itemcount;
    settings_panel_onselect onselect;
};

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