#pragma once

#ifndef NK_NUKLEAR_H_
#include "nuklear.h"
#endif

NK_API void nk_image_padded(struct nk_context *ctx, struct nk_image img, struct nk_vec2 padding);

#ifdef NK_IMPLEMENTATION
NK_API void
nk_image_padded(struct nk_context *ctx, struct nk_image img, struct nk_vec2 padding)
{
    struct nk_window *win;
    struct nk_rect bounds;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return;

    win = ctx->current;
    if (!nk_widget(&bounds, ctx))
        return;
    bounds.x += padding.x;
    bounds.y += padding.y;
    bounds.w -= padding.x * 2;
    bounds.h -= padding.y * 2;
    nk_draw_image(&win->buffer, bounds, &img, nk_white);
}
#endif