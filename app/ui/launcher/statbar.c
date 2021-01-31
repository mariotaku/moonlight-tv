#include "window.h"
#include "strings.h"
#include "priv.h"

#include <string.h>

#include "ui/gui_root.h"
#include "stream/input/absinput.h"

static const struct nk_vec2 statbar_image_padding = nk_vec2_s_const(2, 2);

void launcher_statbar(struct nk_context *ctx)
{
    bool apps_showing = selected_server_node && selected_server_node->server;
    bool is_running = applist_hovered_item && selected_server_node && selected_server_node->server &&
                      applist_hovered_item->id == selected_server_node->server->currentGame;

    struct nk_rect bar_bounds = nk_widget_bounds(ctx);

    float divider_y = bar_bounds.y - ctx->style.window.spacing.y - ctx->style.window.scrollbar_size.y - ctx->style.scrollh.border - 1 * NK_UI_SCALE;
    nk_stroke_line(&ctx->current->buffer, bar_bounds.x, divider_y, bar_bounds.x + bar_bounds.w, divider_y,
                   1 * NK_UI_SCALE, ctx->style.text.color);

    nk_layout_row_template_begin_s(ctx, launcher_bottom_bar_height_dp);
    nk_layout_row_template_push_static_s(ctx, launcher_bottom_bar_height_dp);
    nk_layout_row_template_push_static_s(ctx, 50);
    nk_layout_row_template_push_variable_s(ctx, 1);
    if (_launcher_modals)
    {
    }
    else if (pclist_showing)
    {
        nk_layout_row_template_push_static_s(ctx, launcher_bottom_bar_height_dp);
        nk_layout_row_template_push_static(ctx, nk_string_measure_width(ctx, STR_ACTION_BACK));
        nk_layout_row_template_push_static(ctx, 1);
        nk_layout_row_template_push_static_s(ctx, launcher_bottom_bar_height_dp);
        nk_layout_row_template_push_static(ctx, nk_string_measure_width(ctx, "Confirm"));
    }
    else if (apps_showing)
    {
        nk_layout_row_template_push_static_s(ctx, launcher_bottom_bar_height_dp);
        nk_layout_row_template_push_static(ctx, nk_string_measure_width(ctx, "Change PC"));
        if (is_running)
        {
            nk_layout_row_template_push_static(ctx, 1);
            nk_layout_row_template_push_static_s(ctx, launcher_bottom_bar_height_dp);
            nk_layout_row_template_push_static(ctx, nk_string_measure_width(ctx, "Quit Game"));
        }
        nk_layout_row_template_push_static(ctx, 1);
        nk_layout_row_template_push_static_s(ctx, launcher_bottom_bar_height_dp);
        nk_layout_row_template_push_static(ctx, nk_string_measure_width(ctx, is_running ? STR_ACTION_RESUME : STR_ACTION_LAUNCH));
    }
    else
    {
        nk_layout_row_template_push_static_s(ctx, launcher_bottom_bar_height_dp);
        nk_layout_row_template_push_static(ctx, nk_string_measure_width(ctx, "Select PC"));
    }
    nk_layout_row_template_end(ctx);

    nk_image(ctx, sprites_ui.ic_gamepad);
    nk_labelf(ctx, NK_TEXT_LEFT, "%d", absinput_gamepads());
    nk_spacing(ctx, 1);
    if (_launcher_modals)
    {
    }
    else if (pclist_showing)
    {
        nk_image_padded(ctx, ic_navkey_cancel(), statbar_image_padding);
        nk_label(ctx, STR_ACTION_BACK, NK_TEXT_CENTERED);
        nk_spacing(ctx, 1);
        nk_image_padded(ctx, ic_navkey_confirm(), statbar_image_padding);
        nk_label(ctx, "Confirm", NK_TEXT_CENTERED);
    }
    else if (apps_showing)
    {
        nk_image_padded(ctx, ic_navkey_menu(), statbar_image_padding);
        nk_label(ctx, "Change PC", NK_TEXT_CENTERED);
        if (is_running)
        {
            nk_spacing(ctx, 1);
            nk_image_padded(ctx, ic_navkey_close(), statbar_image_padding);
            nk_label(ctx, "Quit Game", NK_TEXT_CENTERED);
        }
        nk_spacing(ctx, 1);
        nk_image_padded(ctx, ic_navkey_confirm(), statbar_image_padding);
        nk_label(ctx, is_running ? STR_ACTION_RESUME : STR_ACTION_LAUNCH, NK_TEXT_CENTERED);
    }
    else
    {
        nk_image_padded(ctx, ic_navkey_menu(), statbar_image_padding);
        nk_label(ctx, "Select PC", NK_TEXT_CENTERED);
    }
}