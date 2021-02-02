#pragma once

#include <stdbool.h>

#ifndef NK_NUKLEAR_H_
#include "nuklear.h"
#endif

#ifndef NK_UI_SCALE
#warning "NK_UI_SCALE not defined"
#define NK_UI_SCALE 1
#endif

// clang-format off

struct nk_borders {
    float l,t,r,b;
};

NK_API struct nk_vec2 nk_window_get_content_inner_size(struct nk_context *ctx);
NK_API struct nk_borders nk_style_window_get_decoration_size(const struct nk_style *style, enum nk_window_flags flags);
NK_API struct nk_borders nk_style_popup_get_decoration_size(const struct nk_style *style, enum nk_window_flags flags);

NK_API nk_bool nk_checkbox_label_std(struct nk_context*, const char* label, bool *active);

#define nk_rect_s(x, y, w, h) nk_rect((x) * NK_UI_SCALE, (y) * NK_UI_SCALE, (w) * NK_UI_SCALE, (h) * NK_UI_SCALE)
#define nk_rect_s_const(x, y, w, h) {(x) * NK_UI_SCALE, (y) * NK_UI_SCALE, (w) * NK_UI_SCALE, (h) * NK_UI_SCALE}
#define nk_rect_centered(pw, ph, w, h) nk_rect(((pw) - (w)) / 2, ((ph) - (h)) / 2, w, h)
#define nk_rect_s_centered(pw, ph, w, h) nk_rect(((pw) - (w)) / 2.0 * NK_UI_SCALE, ((ph) - (h)) / 2.0 * NK_UI_SCALE, (w)*NK_UI_SCALE, (h)*NK_UI_SCALE)
#define nk_rect_s_centered_size(size, w, h) nk_rect(((size).x - (w) * NK_UI_SCALE) / 2.0, ((size).y - (h) * NK_UI_SCALE) / 2.0, (w)*NK_UI_SCALE, (h)*NK_UI_SCALE)
#define nk_rect_centered_in_rect(bounds, w, h) nk_rect((bounds).x + ((bounds).w - (w)) / 2, (bounds).y + ((bounds).h - (h)) / 2, w, h)
#define nk_rect_s_centered_in_rect(bounds, cw, ch) nk_rect((bounds).x + ((bounds).w - (cw) * NK_UI_SCALE) / 2, (bounds).y + ((bounds).h - (ch) * NK_UI_SCALE) / 2, (cw) * NK_UI_SCALE, (ch) * NK_UI_SCALE)
#define nk_vec2_s(x, y) nk_vec2((x) * NK_UI_SCALE, (y) * NK_UI_SCALE)
#define nk_vec2_s_const(x, y) {(x) * NK_UI_SCALE, (y) * NK_UI_SCALE}

#define nk_rect_center_x(rect) rect.x + rect.w / 2
#define nk_rect_center_y(rect) rect.y + rect.h / 2

#define nk_layout_row_s(ctx, fmt, height, cols, ratio) nk_layout_row(ctx, fmt, (height) * NK_UI_SCALE, cols, ratio)
#define nk_layout_row_dynamic_s(ctx, height, cols) nk_layout_row_dynamic(ctx, (height) * NK_UI_SCALE, cols)
#define nk_layout_row_static_s(ctx, height, item_width, cols) nk_layout_row_static(ctx, (height) * NK_UI_SCALE, (item_width) * NK_UI_SCALE, cols)
#define nk_layout_row_template_begin_s(ctx, h) nk_layout_row_template_begin(ctx, (h) * NK_UI_SCALE)
#define nk_layout_row_template_push_static_s(ctx, width) nk_layout_row_template_push_static(ctx, (width) * NK_UI_SCALE)
#define nk_layout_row_template_push_variable_s(ctx, min_width) nk_layout_row_template_push_variable(ctx, (min_width) * NK_UI_SCALE)
#define nk_layout_space_begin_s(ctx, format, height, widget_count) nk_layout_space_begin(ctx, format, (height) * NK_UI_SCALE, widget_count)

#define nk_list_view_begin_s(ctx, out, id, flags, row_height, row_count) nk_list_view_begin(ctx, out, id, flags, (row_height) * NK_UI_SCALE, row_count)

#define nk_font_atlas_add_from_file_s(atlas, file_path, height, config) nk_font_atlas_add_from_file(atlas, file_path, (height) * NK_UI_SCALE, config)
#define nk_font_atlas_add_from_memory_s(atlas, memory, size, height, config) nk_font_atlas_add_from_memory(atlas, memory, size, (height) * NK_UI_SCALE, config)

// clang-format on

#ifdef NK_IMPLEMENTATION
NK_API struct nk_vec2 nk_window_get_content_inner_size(struct nk_context *ctx)
{
    struct nk_vec2 size = nk_window_get_content_region_size(ctx);
    size.y -= ctx->style.window.padding.y * 2;
    size.x -= ctx->style.window.padding.x * 2;
    return size;
}

NK_API struct nk_borders nk_style_window_get_decoration_size(const struct nk_style *style, enum nk_window_flags flags)
{
    const struct nk_style_window *win = &style->window;
    // left, top, right, bottom
    struct nk_borders borders = {0, 0, 0, 0};
    if (flags & NK_WINDOW_BORDER)
    {
        borders.l += win->border;
        borders.t += win->border;
        borders.r += win->border;
        borders.b += win->border;
    }
    if (flags & NK_WINDOW_TITLE)
    {
        borders.t += style->font->height + 2.0f * win->header.padding.y;
        borders.t += 2.0f * win->header.label_padding.y;
    }
    borders.l += win->padding.x;
    borders.r += win->padding.x;
    borders.t += win->padding.y;
    borders.b += win->padding.y;
    return borders;
}

NK_API struct nk_borders nk_style_popup_get_decoration_size(const struct nk_style *style, enum nk_window_flags flags)
{
    const struct nk_style_window *win = &style->window;
    // left, top, right, bottom
    struct nk_borders borders = {0, 0, 0, 0};
    if (flags & NK_WINDOW_BORDER)
    {
        borders.l += win->popup_border;
        borders.t += win->popup_border;
        borders.r += win->popup_border;
        borders.b += win->popup_border;
    }
    if (flags & NK_WINDOW_TITLE)
    {
        borders.t += style->font->height + 2.0f * win->header.padding.y;
        borders.t += 2.0f * win->header.label_padding.y;
    }
    borders.l += win->popup_padding.x;
    borders.r += win->popup_padding.x;
    borders.t += win->popup_padding.y;
    borders.b += win->popup_padding.y;
    return borders;
}

NK_API nk_bool nk_checkbox_label_std(struct nk_context *ctx, const char *label, bool *value)
{
    nk_bool value_nk = *value ? nk_true : nk_false;
    nk_bool ret = nk_checkbox_label(ctx, label, &value_nk);
    *value = value_nk == nk_true;
    return ret;
}
#endif