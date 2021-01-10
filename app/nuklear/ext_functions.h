#ifndef NK_NUKLEAR_H_
#include "nuklear.h"
#endif

#ifndef NK_UI_SCALE
#warning "NK_UI_SCALE not defined"
#define NK_UI_SCALE 1
#endif

// clang-format off

NK_API struct nk_vec2 nk_window_get_content_inner_size(struct nk_context *ctx);

#define nk_rect_s(x, y, w, h) nk_rect((x) * NK_UI_SCALE, (y) * NK_UI_SCALE, (w) * NK_UI_SCALE, (h) * NK_UI_SCALE)
#define nk_rect_s_const(x, y, w, h) {(x) * NK_UI_SCALE, (y) * NK_UI_SCALE, (w) * NK_UI_SCALE, (h) * NK_UI_SCALE}
#define nk_rect_centered(pw, ph, w, h) nk_rect(((pw) - (w)) / 2, ((ph) - (h)) / 2, w, h)
#define nk_rect_s_centered(pw, ph, w, h) nk_rect(((pw) - (w)) / 2.0 * NK_UI_SCALE, ((ph) - (h)) / 2.0 * NK_UI_SCALE, (w)*NK_UI_SCALE, (h)*NK_UI_SCALE)
#define nk_vec2_s(x, y) nk_vec2((x) * NK_UI_SCALE, (y) * NK_UI_SCALE)
#define nk_vec2_s_const(x, y) {(x) * NK_UI_SCALE, (y) * NK_UI_SCALE}

#define nk_layout_row_s(ctx, fmt, height, cols, ratio) nk_layout_row(ctx, fmt, (height) * NK_UI_SCALE, cols, ratio)
#define nk_layout_row_dynamic_s(ctx, height, cols) nk_layout_row_dynamic(ctx, (height) * NK_UI_SCALE, cols)
#define nk_layout_row_static_s(ctx, height, item_width, cols) nk_layout_row_static(ctx, (height) * NK_UI_SCALE, (item_width) * NK_UI_SCALE, cols)
#define nk_layout_row_template_begin_s(ctx, h) nk_layout_row_template_begin(ctx, (h) * NK_UI_SCALE)
#define nk_layout_row_template_push_static_s(ctx, width) nk_layout_row_template_push_static(ctx, (width) * NK_UI_SCALE)
#define nk_layout_row_template_push_variable_s(ctx, min_width) nk_layout_row_template_push_variable(ctx, (min_width) * NK_UI_SCALE)

#define nk_list_view_begin_s(ctx, out, id, flags, row_height, row_count) nk_list_view_begin(ctx, out, id, flags, (row_height) * NK_UI_SCALE, row_count)

#define nk_font_atlas_add_from_file_s(atlas, file_path, height, config) nk_font_atlas_add_from_file(atlas, file_path, (height) * NK_UI_SCALE, config)

// clang-format on

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