#include "window.h"
#include "priv.h"
#include "ui/root.h"
#include "ui/fonts.h"

#include <stddef.h>
#include <stdio.h>

#define HINT_SOPS "May not work under some resolutions or framerates. Moonlight will try continue without optimization."

static int hint_sops_width, hint_sops_height = 0;

static bool _settings_pane_host(struct nk_context *ctx, bool *showing_combo)
{
    static struct nk_rect item_bounds = {0, 0, 0, 0};
    int item_index = 0;

    nk_layout_row_dynamic_s(ctx, 25, 1);
    nk_label(ctx, "Host Settings", NK_TEXT_LEFT);
    nk_layout_row_dynamic_s(ctx, 25, 1);

    int w = app_configuration->stream.width, h = app_configuration->stream.height,
        fps = app_configuration->stream.fps;
    settings_item_update_selected_bounds(ctx, item_index++, &item_bounds);
    nk_checkbox_label_std(ctx, "Optimize game settings for streaming", &app_configuration->sops);

    nk_style_push_font(ctx, &font_ui_15->handle);
    if (!hint_sops_width || !hint_sops_height)
    {
        hint_sops_width = nk_widget_width(ctx);
        hint_sops_width -= ctx->style.window.spacing.x;
        hint_sops_width -= 20 * ui_scale;
        hint_sops_height = nk_text_wrap_measure_height(ctx, hint_sops_width, HINT_SOPS, strlen(HINT_SOPS)) + 2 * ui_scale;
    }
    nk_layout_row_template_begin(ctx, hint_sops_height);
    nk_layout_row_template_push_static_s(ctx, 20);
    nk_layout_row_template_push_static(ctx, hint_sops_width);
    nk_layout_row_template_end(ctx);
    nk_spacing(ctx, 1);
    nk_label_wrap(ctx, HINT_SOPS);
    nk_style_pop_font(ctx);

    nk_layout_row_dynamic_s(ctx, 25, 1);

    settings_item_update_selected_bounds(ctx, item_index++, &item_bounds);
    nk_checkbox_label_std(ctx, "Play audio on host PC", &app_configuration->localaudio);

    settings_item_update_selected_bounds(ctx, item_index++, &item_bounds);
    nk_checkbox_label_std(ctx, "Disable all input processing (view-only mode)", &app_configuration->viewonly);

    settings_item_update_selected_bounds(ctx, item_index++, &item_bounds);
    nk_checkbox_label_std(ctx, "HDR (Experimental)", &app_configuration->stream.enableHdr);
    return true;
}

int _settings_pane_host_itemcount()
{
    return 4;
}

static void _onselect(struct nk_context *ctx)
{
    hint_sops_width = 0;
    hint_sops_height = 0;
}

struct settings_pane settings_pane_host = {
    .title = "Host Settings",
    .render = _settings_pane_host,
    .navkey = NULL,
    .itemcount = _settings_pane_host_itemcount,
    .onselect = _onselect,
};