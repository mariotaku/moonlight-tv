#include "window.h"
#include "priv.h"
#include "ui/root.h"
#include "ui/fonts.h"

#include <stddef.h>
#include <stdio.h>

static int _item_count = 0;

static bool _render(struct nk_context *ctx, bool *showing_combo)
{
    static struct nk_rect item_bounds = {0, 0, 0, 0};
    _item_count = 0;

    nk_layout_row_dynamic_s(ctx, 25, 1);

    settings_item_update_selected_bounds(ctx, _item_count++, &item_bounds);
    nk_checkbox_label_std(ctx, "Disable all input processing (view-only mode)", &app_configuration->viewonly);

#ifndef TARGET_WEBOS
    settings_item_update_selected_bounds(ctx, _item_count++, &item_bounds);
    nk_checkbox_label_std(ctx, "Absolute mouse", &app_configuration->absmouse);
#endif

    settings_item_update_selected_bounds(ctx, _item_count++, &item_bounds);
    nk_checkbox_label_std(ctx, "Gamepad has ABXY swapped", &app_configuration->swap_abxy);
    return true;
}

static int _itemcount()
{
    return _item_count;
}

static void _onselect(struct nk_context *ctx)
{
}

struct settings_pane settings_pane_input = {
    .title = "Input Settings",
    .render = _render,
    .navkey = NULL,
    .itemcount = _itemcount,
    .onselect = _onselect,
};