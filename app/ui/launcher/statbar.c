#include "window.h"

#include <string.h>

#include "ui/gui_root.h"
#include "stream/input/absinput.h"

static inline struct nk_image ic_navkey_close()
{
    switch (ui_input_mode)
    {
    case UI_INPUT_MODE_POINTER:
    case UI_INPUT_MODE_KEY:
        return sprites_ui.ic_remote_red;
    case UI_INPUT_MODE_GAMEPAD:
        return sprites_ui.ic_gamepad_x;
    }
};

static inline struct nk_image ic_navkey_menu()
{
    switch (ui_input_mode)
    {
    case UI_INPUT_MODE_POINTER:
    case UI_INPUT_MODE_KEY:
        return sprites_ui.ic_remote_yellow;
    case UI_INPUT_MODE_GAMEPAD:
        return sprites_ui.ic_gamepad_view;
    }
};

static inline struct nk_image ic_navkey_confirm()
{
    switch (ui_input_mode)
    {
    case UI_INPUT_MODE_POINTER:
    case UI_INPUT_MODE_KEY:
        return sprites_ui.ic_remote_center;
    case UI_INPUT_MODE_GAMEPAD:
        return sprites_ui.ic_gamepad_a;
    }
};

void launcher_statbar(struct nk_context *ctx)
{
    bool show_actions = applist_hovered_item != NULL || ui_input_mode == UI_INPUT_MODE_POINTER;
    bool is_running = applist_hovered_item && selected_server_node && selected_server_node->server &&
                      applist_hovered_item->id == selected_server_node->server->currentGame;

    nk_layout_row_template_begin_s(ctx, launcher_bottom_bar_height_dp);
    nk_layout_row_template_push_static_s(ctx, launcher_bottom_bar_height_dp);
    nk_layout_row_template_push_static_s(ctx, 50);
    nk_layout_row_template_push_variable_s(ctx, 1);
    if (show_actions)
    {
        nk_layout_row_template_push_static_s(ctx, launcher_bottom_bar_height_dp);
        nk_layout_row_template_push_static(ctx, nk_string_measure_width(ctx, "Select PC"));
        if (is_running)
        {
            nk_layout_row_template_push_static_s(ctx, 1);
            nk_layout_row_template_push_static_s(ctx, launcher_bottom_bar_height_dp);
            nk_layout_row_template_push_static(ctx, nk_string_measure_width(ctx, "Close"));
        }
        nk_layout_row_template_push_static_s(ctx, 1);
        nk_layout_row_template_push_static_s(ctx, launcher_bottom_bar_height_dp);
        if (is_running)
        {
            nk_layout_row_template_push_static(ctx, nk_string_measure_width(ctx, "Resume"));
        }
        else
        {
            nk_layout_row_template_push_static(ctx, nk_string_measure_width(ctx, "Launch"));
        }
    }
    nk_layout_row_template_end(ctx);

    nk_image(ctx, sprites_ui.ic_gamepad);
    nk_labelf(ctx, NK_TEXT_LEFT, "%d", absinput_gamepads());
    nk_spacing(ctx, 1);
    if (show_actions)
    {
        nk_image_padded(ctx, ic_navkey_menu(), nk_vec2_s(5, 5));
        nk_label(ctx, "Select PC", NK_TEXT_RIGHT);
        if (is_running)
        {
            nk_spacing(ctx, 1);
            nk_image_padded(ctx, ic_navkey_close(), nk_vec2_s(5, 5));
            nk_label(ctx, "Close", NK_TEXT_RIGHT);
        }
        nk_spacing(ctx, 1);
        nk_image_padded(ctx, ic_navkey_confirm(), nk_vec2_s(5, 5));
        if (is_running)
        {
            nk_label(ctx, "Resume", NK_TEXT_RIGHT);
        }
        else
        {
            nk_label(ctx, "Launch", NK_TEXT_RIGHT);
        }
    }
}