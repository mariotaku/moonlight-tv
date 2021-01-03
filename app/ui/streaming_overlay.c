#include "streaming_overlay.h"

static void connection_dialog(struct nk_context *ctx, STREAMING_STATUS stat);

void streaming_overlay_init(struct nk_context *ctx)
{
}

bool streaming_overlay(struct nk_context *ctx, STREAMING_STATUS stat)
{
    if (stat != STREAMING_STREAMING)
    {
        connection_dialog(ctx, stat);
    }
    return true;
}

void connection_dialog(struct nk_context *ctx, STREAMING_STATUS stat)
{
    struct nk_vec2 win_region_size;
    int content_height_remaining;
    if (nk_begin(ctx, "Connection", nk_rect(330, 240, 300, 60), NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR))
    {
        win_region_size = nk_window_get_content_region_size(ctx);
        content_height_remaining = (int)win_region_size.y;
        content_height_remaining -= ctx->style.window.padding.y * 2;
        content_height_remaining -= (int)ctx->style.window.border;
        nk_layout_row_dynamic(ctx, content_height_remaining, 1);

        switch (stat)
        {
        case STREAMING_CONNECTING:
            nk_label(ctx, "Connecting...", NK_TEXT_ALIGN_LEFT);
            break;
        case STREAMING_DISCONNECTING:
            nk_label(ctx, "Disconnecting...", NK_TEXT_ALIGN_LEFT);
            break;
        }
    }
    nk_end(ctx);
}