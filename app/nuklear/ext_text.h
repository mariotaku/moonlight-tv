#pragma once
#ifndef NK_NUKLEAR_H_
#include "nuklear.h"
#endif

#ifndef NK_UI_SCALE
#warning "NK_UI_SCALE not defined"
#define NK_UI_SCALE 1
#endif

#define nk_string_measure_width(ctx, string) nk_text_measure_width(ctx, string, strlen(string))

NK_API float nk_text_measure_width(struct nk_context *ctx, const char *text, int len);

NK_API int nk_text_wrap_measure_height(struct nk_context *ctx, int max_width,
                                       const char *text, int len);

#ifdef NK_IMPLEMENTATION

NK_API float nk_text_measure_width(struct nk_context *ctx, const char *text, int len)
{
    const struct nk_window *win = ctx->current;
    const struct nk_style style = ctx->style;
    const struct nk_user_font *f = style.font;
    const struct nk_vec2 text_padding = style.text.padding;
    int row_height = f->height + 1;
    int glyphs = 0;
    const char *remaining;
    const struct nk_vec2 size = nk_text_calculate_text_bounds(f, text, len, row_height, &remaining, 0,
                                                              &glyphs, NK_STOP_ON_NEW_LINE);
    return size.x + 2 * text_padding.x;
}

NK_API int nk_text_wrap_measure_height(struct nk_context *ctx, int max_width,
                                       const char *text, int len)
{
    const struct nk_window *win = ctx->current;
    const struct nk_style style = ctx->style;
    const struct nk_user_font *f = style.font;
    const struct nk_vec2 text_padding = style.text.padding;

    float width;
    int glyphs = 0;
    int fitting = 0;
    int done = 0;
    struct nk_rect line;
    NK_INTERN nk_rune seperator[] = {' '};

    float text_content_width = max_width - 2 * text_padding.x;
    float total_height = 0;

    fitting = nk_text_clamp(f, text, len, text_content_width, &glyphs, &width, seperator, NK_LEN(seperator));
    while (done < len)
    {
        if (!fitting)
            break;
        done += fitting;
        total_height += f->height + 2 * text_padding.y;
        fitting = nk_text_clamp(f, &text[done], len - done, text_content_width, &glyphs, &width, seperator, NK_LEN(seperator));
    }
    return total_height;
}

#endif