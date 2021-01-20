#pragma once
#ifndef NK_NUKLEAR_H_
#include "nuklear.h"
#endif

#ifndef NK_UI_SCALE
#warning "NK_UI_SCALE not defined"
#define NK_UI_SCALE 1
#endif

NK_API int nk_text_wrap_measure_height(struct nk_context *ctx, int max_width,
                                       const char *string, int len);

#ifdef NK_IMPLEMENTATION

NK_API int nk_text_wrap_measure_height(struct nk_context *ctx, int max_width,
                                       const char *string, int len)
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

    fitting = nk_text_clamp(f, string, len, text_content_width, &glyphs, &width, seperator, NK_LEN(seperator));
    while (done < len)
    {
        if (!fitting)
            break;
        done += fitting;
        total_height += f->height + 2 * text_padding.y;
        fitting = nk_text_clamp(f, &string[done], len - done, text_content_width, &glyphs, &width, seperator, NK_LEN(seperator));
    }
    return total_height;
}

#endif