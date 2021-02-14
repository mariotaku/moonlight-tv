#include "window.h"

#include <string.h>

#include "ui/root.h"
#include "ui/strings.h"

void settings_statbar(struct nk_context *ctx)
{
    struct nk_rect bar_bounds = nk_widget_bounds(ctx);
    bar_bounds.w = nk_layout_widget_bounds(ctx).w;

    float divider_y = bar_bounds.y - ctx->style.window.spacing.y - ctx->style.window.scrollbar_size.y - ctx->style.scrollh.border - 1 * NK_UI_SCALE;
    nk_stroke_line(&ctx->current->buffer, bar_bounds.x, divider_y, bar_bounds.x + bar_bounds.w, divider_y,
                   1 * NK_UI_SCALE, ctx->style.text.color);

    nk_layout_row_template_begin_s(ctx, UI_BOTTOM_BAR_HEIGHT_DP);
    nk_layout_row_template_push_variable_s(ctx, 1);
    nk_layout_row_template_push_static_s(ctx, UI_BOTTOM_BAR_HEIGHT_DP);
    nk_layout_row_template_push_static(ctx, nk_string_measure_width(ctx, STR_ACTION_BACK));
    nk_layout_row_template_end(ctx);

    nk_spacing(ctx, 1);
    nk_image_padded(ctx, ic_navkey_cancel(), ui_statbar_icon_padding);
    nk_label(ctx, STR_ACTION_BACK, NK_TEXT_CENTERED);
}