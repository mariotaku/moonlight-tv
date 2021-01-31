#include "overlay.h"
#include "priv.h"

#include "ui/gui_root.h"
#include "stream/input/absinput.h"

static const struct nk_vec2 statbar_image_padding = nk_vec2_s_const(2, 2);
#define overlay_bottom_bar_height_dp 20

void _streaming_bottom_bar(struct nk_context *ctx)
{
    const int bar_height = 50 * NK_UI_SCALE;
    struct nk_style_item window_bg = ctx->style.window.fixed_background;
    window_bg.data.color.a = 160;
    nk_style_push_style_item(ctx, &ctx->style.window.fixed_background, window_bg);
    nk_style_push_vec2(ctx, &ctx->style.window.padding, nk_vec2_s(20, 10));
    if (nk_begin(ctx, "Overlay BottomBar", nk_rect(0, gui_display_height - bar_height, gui_display_width, bar_height), NK_WINDOW_NO_SCROLLBAR))
    {
        nk_style_push_style_item(ctx, &ctx->style.window.fixed_background, nk_style_item_color(nk_rgba(0, 0, 0, 0)));
        nk_style_push_float(ctx, &ctx->style.window.group_border, 0);
        nk_style_push_float(ctx, &ctx->style.window.border, 0);

        nk_layout_space_begin_s(ctx, NK_STATIC, nk_window_get_content_region_size(ctx).y, 2);
        struct nk_rect region = nk_layout_space_bounds(ctx);

        struct nk_vec2 action_button_padding = nk_vec2(5 * NK_UI_SCALE, 0);
        float action_spacing = ctx->style.window.spacing.x;
        float action_icon_width = overlay_bottom_bar_height_dp * NK_UI_SCALE;
        float quit_label_width = nk_string_measure_width(ctx, "Quit Game");
        float back_label_width = nk_string_measure_width(ctx, "Games List");
        float bar_content_height = bar_height - ctx->style.window.padding.y * 2;

        nk_layout_space_push(ctx, nk_rect(0, 0, region.w, bar_content_height));

        nk_style_push_vec2(ctx, &ctx->style.window.group_padding, nk_vec2(0, 0));
        if (nk_group_begin(ctx, "overlay_bottom_buttons", NK_WINDOW_NO_SCROLLBAR))
        {
            nk_layout_row_template_begin_s(ctx, 30);
            nk_layout_row_template_push_variable(ctx, 1);
            nk_layout_row_template_push_static(ctx, action_button_padding.x * 2 + action_icon_width + action_spacing + back_label_width);
            nk_layout_row_template_push_static(ctx, 1);
            nk_layout_row_template_push_static(ctx, action_button_padding.x * 2 + action_icon_width + action_spacing + quit_label_width);
            nk_layout_row_template_end(ctx);

            nk_spacing(ctx, 1);
            struct nk_rect bounds = nk_widget_bounds(ctx);
            _btn_suspend_center.x = nk_rect_center_x(bounds);
            _btn_suspend_center.y = nk_rect_center_y(bounds);
            if (nk_button_label(ctx, ""))
            {
                streaming_interrupt(false);
                stream_overlay_showing = false;
            }

            nk_spacing(ctx, 1);
            bounds = nk_widget_bounds(ctx);
            _btn_quit_center.x = nk_rect_center_x(bounds);
            _btn_quit_center.y = nk_rect_center_y(bounds);
            if (nk_button_label(ctx, ""))
            {
                streaming_interrupt(true);
                stream_overlay_showing = false;
            }
            nk_group_end(ctx);
        }
        nk_style_pop_vec2(ctx);

        nk_layout_space_push(ctx, nk_rect(0, 0, region.w, bar_content_height));

        nk_style_push_vec2(ctx, &ctx->style.window.group_padding, nk_vec2_s(0, 5));
        if (nk_group_begin(ctx, "overlay_bottom_actions", NK_WINDOW_NO_SCROLLBAR))
        {
            nk_layout_row_template_begin_s(ctx, overlay_bottom_bar_height_dp);
            nk_layout_row_template_push_static_s(ctx, overlay_bottom_bar_height_dp);
            nk_layout_row_template_push_static_s(ctx, 50);
            nk_layout_row_template_push_variable(ctx, 1);
            nk_layout_row_template_push_static(ctx, action_icon_width);
            nk_layout_row_template_push_static(ctx, back_label_width + action_button_padding.x);
            nk_layout_row_template_push_static(ctx, action_button_padding.x + 1);
            nk_layout_row_template_push_static(ctx, action_icon_width);
            nk_layout_row_template_push_static(ctx, quit_label_width + action_button_padding.x);
            nk_layout_row_template_end(ctx);

            nk_image(ctx, sprites_ui.ic_gamepad);
            nk_labelf(ctx, NK_TEXT_LEFT, "%d", absinput_gamepads());

            nk_spacing(ctx, 1);
            nk_image_padded(ctx, ic_navkey_menu(), statbar_image_padding);
            nk_label(ctx, "Games List", NK_TEXT_LEFT);
            nk_spacing(ctx, 1);
            nk_image_padded(ctx, ic_navkey_close(), statbar_image_padding);
            nk_label(ctx, "Quit Game", NK_TEXT_LEFT);

            nk_group_end(ctx);
        }
        nk_style_pop_vec2(ctx);

        nk_layout_space_end(ctx);

        nk_style_pop_float(ctx);
        nk_style_pop_float(ctx);
        nk_style_pop_style_item(ctx);
    }
    nk_end(ctx);
    nk_style_pop_vec2(ctx);
    nk_style_pop_style_item(ctx);
}