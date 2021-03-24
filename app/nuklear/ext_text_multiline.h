#pragma once
#ifndef NK_NUKLEAR_H_
#include "nuklear.h"
#endif

NK_API void nk_text_multiline_colored(struct nk_context *ctx, const char *str,
    int len, struct nk_color color);

NK_API void nk_text_multiline(struct nk_context *ctx, const char *str, int len);

NK_API void nk_label_multiline(struct nk_context *ctx, const char *str);

NK_API int nk_text_multiline_measure_height(struct nk_context *ctx, int max_width,
                                       const char *text, int len);

#ifdef NK_IMPLEMENTATION

NK_LIB void nk_widget_text_multiline(struct nk_command_buffer *o, struct nk_rect b,
    const char *string, int len, const struct nk_text *t,
    const struct nk_user_font *f);

NK_LIB int nk_text_clamp_multiline(const struct nk_user_font *font, const char *text,
    int text_len, float space, int *glyphs, float *text_width, int *line_len,
    nk_rune *sep_list, int sep_count);

NK_API void
nk_text_multiline_colored(struct nk_context *ctx, const char *str,
    int len, struct nk_color color)
{
    struct nk_window *win;
    const struct nk_style *style;

    struct nk_vec2 item_padding;
    struct nk_rect bounds;
    struct nk_text text;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout) return;

    win = ctx->current;
    style = &ctx->style;
    nk_panel_alloc_space(&bounds, ctx);
    item_padding = style->text.padding;

    text.padding.x = item_padding.x;
    text.padding.y = item_padding.y;
    text.background = style->window.background;
    text.text = color;
    nk_widget_text_multiline(&win->buffer, bounds, str, len, &text, style->font);
}

NK_API void
nk_text_multiline(struct nk_context *ctx, const char *str, int len)
{
    NK_ASSERT(ctx);
    if (!ctx) return;
    nk_text_multiline_colored(ctx, str, len, ctx->style.text.color);
}

NK_API void
nk_label_multiline(struct nk_context *ctx, const char *str)
{
    nk_text_multiline(ctx, str, nk_strlen(str));
}

NK_API int nk_text_multiline_measure_height(struct nk_context *ctx, int max_width,
                                       const char *text, int len)
{
    const struct nk_window *win = ctx->current;
    const struct nk_style style = ctx->style;
    const struct nk_user_font *f = style.font;
    const struct nk_vec2 text_padding = style.text.padding;

    float width;
    int glyphs = 0;
    int fitting = 0;
    int line_len = 0;
    int done = 0;
    struct nk_rect line;
    NK_INTERN nk_rune seperator[] = {' '};

    float text_content_width = max_width - 2 * text_padding.x;
    float total_height = 0;

    fitting = nk_text_clamp_multiline(f, text, len, text_content_width, &glyphs, &width, &line_len, seperator, NK_LEN(seperator));
    while (done < len)
    {
        if (!fitting)
            break;
        done += fitting;
        total_height += f->height + 2 * text_padding.y;
        fitting = nk_text_clamp_multiline(f, &text[done], len - done, text_content_width, &glyphs, &width, &line_len, seperator, NK_LEN(seperator));
    }
    return total_height;
}

NK_LIB void
nk_widget_text_multiline(struct nk_command_buffer *o, struct nk_rect b,
    const char *string, int len, const struct nk_text *t,
    const struct nk_user_font *f)
{
    float width;
    int glyphs = 0;
    int fitting = 0;
    int line_len = 0;
    int done = 0;
    struct nk_rect line;
    struct nk_text text;
    NK_INTERN nk_rune seperator[] = {' '};

    NK_ASSERT(o);
    NK_ASSERT(t);
    if (!o || !t) return;

    text.padding = nk_vec2(0,0);
    text.background = t->background;
    text.text = t->text;

    b.w = NK_MAX(b.w, 2 * t->padding.x);
    b.h = NK_MAX(b.h, 2 * t->padding.y);
    b.h = b.h - 2 * t->padding.y;

    line.x = b.x + t->padding.x;
    line.y = b.y + t->padding.y;
    line.w = b.w - 2 * t->padding.x;
    line.h = 2 * t->padding.y + f->height;

    fitting = nk_text_clamp_multiline(f, string, len, line.w, &glyphs, &width, &line_len, seperator,NK_LEN(seperator));
    while (done < len) {
        if (!fitting || line.y + line.h >= (b.y + b.h)) break;
        nk_widget_text(o, line, &string[done], line_len, &text, NK_TEXT_LEFT, f);
        done += fitting;
        line.y += f->height + 2 * t->padding.y;
        fitting = nk_text_clamp_multiline(f, &string[done], len - done, line.w, &glyphs, &width, &line_len, seperator,NK_LEN(seperator));
    }
}

NK_LIB int
nk_text_clamp_multiline(const struct nk_user_font *font, const char *text,
    int text_len, float space, int *glyphs, float *text_width, int *line_len,
    nk_rune *sep_list, int sep_count)
{
    int i = 0;
    int glyph_len = 0;
    float last_width = 0;
    nk_rune unicode = 0;
    float width = 0;
    int len = 0;
    int g = 0;
    float s;

    int sep_len = 0;
    int sep_g = 0;
    float sep_width = 0;
    sep_count = NK_MAX(sep_count,0);

    glyph_len = nk_utf_decode(text, &unicode, text_len);
    int linebreak = 0;
    while (glyph_len && (width < space) && (len < text_len)) {
        len += glyph_len;
        if (unicode == '\n') {
            sep_width = last_width = width;
            sep_g = g+1;
            sep_len = len;
            linebreak = linebreak+1;
            break;
        }
        s = font->width(font->userdata, font->height, text, len);
        for (i = 0; i < sep_count; ++i) {
            if (unicode != sep_list[i]) continue;
            sep_width = last_width = width;
            sep_g = g+1;
            sep_len = len;
            break;
        }
        if (i == sep_count){
            last_width = sep_width = width;
            sep_g = g+1;
        }
        width = s;
        glyph_len = nk_utf_decode(&text[len], &unicode, text_len - len);
        g++;
    }
    if (len >= text_len) {
        *glyphs = g;
        *text_width = last_width;
        *line_len = len - linebreak;
        return len;
    } else {
        *glyphs = sep_g;
        *text_width = sep_width;
        int result = (!sep_len) ? len: sep_len;
        *line_len = result - linebreak;
        return result;
    }
}


#endif