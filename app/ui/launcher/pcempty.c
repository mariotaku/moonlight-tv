#include "window.h"

bool launcher_pcempty(struct nk_context *ctx, PSERVER_LIST node, bool event_emitted)
{
  float group_height = nk_widget_height(ctx);
  if (nk_group_begin(ctx, "launcher_empty", 0))
  {
    group_height -= ctx->style.window.group_padding.y * 2;
    const float ratio[4] = {0.1, 0.27, 0.53, 0.1};
    nk_layout_row(ctx, NK_DYNAMIC, group_height, 4, ratio);
    nk_spacing(ctx, 1);
    struct nk_vec2 group_size = nk_widget_size(ctx);
    if (nk_group_begin(ctx, "launcher_empty_icon", NK_WINDOW_NO_SCROLLBAR))
    {
      struct nk_rect desktop_bounds = nk_rect_s_centered_size(group_size, 96, 96);
      nk_layout_space_begin(ctx, NK_STATIC, 0, 2);
      nk_layout_space_push(ctx, desktop_bounds);
      nk_image(ctx, sprites_ui.ic_desktop_large);
      struct nk_rect stat_bounds = nk_rect_s_centered_in_rect(desktop_bounds, 48, 48);
      stat_bounds.y -= 8 * NK_UI_SCALE;
      nk_layout_space_push(ctx, stat_bounds);
      nk_image(ctx, sprites_ui.ic_warning);
      nk_layout_space_end(ctx);
      nk_group_end(ctx);
    }
    if (nk_group_begin(ctx, "launcher_empty_message", NK_WINDOW_NO_SCROLLBAR))
    {
      const int actions_count = 2;
      int actions_height = 25 * NK_UI_SCALE * actions_count + ctx->style.window.spacing.y * (actions_count - 1);
      nk_layout_row_dynamic(ctx, (group_height) / 2 - actions_height, 1);
      // nk_layout_row(ctx, NK_DYNAMIC, );
      nk_label(ctx, "PC is not responding", NK_TEXT_ALIGN_CENTERED | NK_TEXT_ALIGN_BOTTOM);
      nk_layout_row_dynamic_s(ctx, 10, 1);
      nk_spacing(ctx, 1);

      nk_layout_row_template_begin_s(ctx, actions_height);
      nk_layout_row_template_push_variable(ctx, 1);
      nk_layout_row_template_push_static_s(ctx, 150);
      nk_layout_row_template_push_variable(ctx, 1);
      nk_layout_row_template_end(ctx);

      nk_spacing(ctx, 1);

      if (nk_group_begin(ctx, "launcher_empty_actions", NK_WINDOW_NO_SCROLLBAR))
      {
        nk_layout_row_dynamic_s(ctx, 25, 1);
        if (nk_button_label(ctx, "Send wake signal"))
        {
          pcmanager_send_wol(node->server);
        }
        if (nk_button_label(ctx, "Retry loading"))
        {
          pcmanager_request_update(node->server);
        }
        nk_group_end(ctx);
      }

      nk_spacing(ctx, 1);
      nk_group_end(ctx);
    }
    nk_spacing(ctx, 1);
    nk_group_end(ctx);
  }
  return false;
}