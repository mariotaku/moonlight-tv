#ifndef NK_NUKLEAR_H_
#include "nuklear.h"
#endif

NK_API struct nk_vec2 nk_window_get_content_inner_size(struct nk_context *ctx);

#ifdef NK_IMPLEMENTATION
NK_API struct nk_vec2 nk_window_get_content_inner_size(struct nk_context *ctx)
{
    struct nk_vec2 size = nk_window_get_content_region_size(ctx);
    size.y -= ctx->style.window.padding.y * 2;
    size.y -= ctx->style.window.border * 2;
    size.x -= ctx->style.window.padding.x * 2;
    size.x -= ctx->style.window.border * 2;
    return size;
}
#endif