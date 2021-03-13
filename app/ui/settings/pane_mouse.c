#include "window.h"

#include <stdlib.h>
#include <stdio.h>

bool _settings_pane_mouse(struct nk_context *ctx, bool *showing_combo)
{
    static struct nk_rect item_bounds = {0, 0, 0, 0};
    int item_index = 0;

    nk_layout_row_dynamic_s(ctx, 25, 1);
    nk_label(ctx, "Absolute mouse mapping", NK_TEXT_LEFT);

    nk_layout_row_template_begin_s(ctx, 25);
    nk_layout_row_template_push_static_s(ctx, 120);
    nk_layout_row_template_push_dynamic(ctx);
    nk_layout_row_template_push_dynamic(ctx);
    nk_layout_row_template_end(ctx);
    nk_label(ctx, "Desktop size", NK_TEXT_LEFT);
    settings_item_update_selected_bounds(ctx, item_index++, &item_bounds);
    nk_property_int(ctx, "w:", 0, &app_configuration->absmouse_mapping.desktop_w, 12800, 1, 0);
    settings_item_update_selected_bounds(ctx, item_index++, &item_bounds);
    nk_property_int(ctx, "h:", 0, &app_configuration->absmouse_mapping.desktop_h, 12800, 1, 0);

    nk_layout_row_template_begin_s(ctx, 25);
    nk_layout_row_template_push_static_s(ctx, 120);
    nk_layout_row_template_push_dynamic(ctx);
    nk_layout_row_template_push_dynamic(ctx);
    nk_layout_row_template_end(ctx);
    nk_label(ctx, "Screen size", NK_TEXT_LEFT);
    settings_item_update_selected_bounds(ctx, item_index++, &item_bounds);
    nk_property_int(ctx, "w: ", 0, &app_configuration->absmouse_mapping.screen_w, 12800, 1, 0);
    settings_item_update_selected_bounds(ctx, item_index++, &item_bounds);
    nk_property_int(ctx, "h: ", 0, &app_configuration->absmouse_mapping.screen_h, 12800, 1, 0);

    nk_layout_row_template_begin_s(ctx, 25);
    nk_layout_row_template_push_static_s(ctx, 120);
    nk_layout_row_template_push_dynamic(ctx);
    nk_layout_row_template_push_dynamic(ctx);
    nk_layout_row_template_end(ctx);
    nk_label(ctx, "Screen position", NK_TEXT_LEFT);
    settings_item_update_selected_bounds(ctx, item_index++, &item_bounds);
    nk_property_int(ctx, "x:", 0, &app_configuration->absmouse_mapping.screen_x, 12800, 1, 0);
    settings_item_update_selected_bounds(ctx, item_index++, &item_bounds);
    nk_property_int(ctx, "y:", 0, &app_configuration->absmouse_mapping.screen_y, 12800, 1, 0);

    return true;
}

int _settings_pane_mouse_itemcount()
{
    return 6;
}

struct settings_pane settings_pane_mouse = {
    .title = "Mouse Settings",
    .render = _settings_pane_mouse,
    .navkey = NULL,
    .itemcount = _settings_pane_mouse_itemcount,
    .onselect = NULL,
};
