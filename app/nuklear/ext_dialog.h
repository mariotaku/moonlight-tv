#pragma once
#ifndef NK_NUKLEAR_H_
#include "nuklear.h"
#include "ext_functions.h"
#include "ext_text.h"
#include "ext_text_multiline.h"
#endif

#ifndef NK_UI_SCALE
#warning "NK_UI_SCALE not defined"
#define NK_UI_SCALE 1
#endif

enum nk_dialog_result
{
    NK_DIALOG_CANCELLED = -1,
    NK_DIALOG_NONE = 0,
    NK_DIALOG_RUNNING = 1,
    NK_DIALOG_POSITIVE = 10,
    NK_DIALOG_NEGATIVE,
    NK_DIALOG_NEUTRAL
};

enum nk_dialog_result nk_dialog_begin(struct nk_context *ctx, int container_width, int container_height, const char *title,
                                      const char *message, const char *positive, const char *negative, const char *neutral);

enum nk_dialog_result nk_dialog_popup_begin(struct nk_context *ctx, const char *title, const char *message,
                                            const char *positive, const char *negative, const char *neutral);

#ifdef NK_IMPLEMENTATION

inline static enum nk_dialog_result _nk_dialog_content(struct nk_context *ctx, const char *message, int message_height,
                                                       const char *positive, const char *negative, const char *neutral)
{
    nk_layout_row_dynamic(ctx, message_height, 1);
    nk_label_multiline(ctx, message);

    nk_layout_row_template_begin_s(ctx, 30);
    nk_layout_row_template_push_static_s(ctx, 5);
    if (neutral)
    {
        nk_layout_row_template_push_static_s(ctx, 80);
    }
    nk_layout_row_template_push_variable_s(ctx, 10);
    if (negative)
    {
        nk_layout_row_template_push_static_s(ctx, 80);
    }
    if (positive)
    {
        nk_layout_row_template_push_static_s(ctx, 80);
    }
    nk_layout_row_template_push_static_s(ctx, 5);
    nk_layout_row_template_end(ctx);

    nk_spacing(ctx, 1);
    enum nk_dialog_result result = NK_DIALOG_RUNNING;
    if (neutral && nk_button_label(ctx, neutral))
    {
        result = NK_DIALOG_NEUTRAL;
    }
    nk_spacing(ctx, 1);
    if (negative && nk_button_label(ctx, negative))
    {
        result = NK_DIALOG_NEGATIVE;
    }
    if (positive && nk_button_label(ctx, positive))
    {
        result = NK_DIALOG_POSITIVE;
    }
    nk_spacing(ctx, 1);
    nk_layout_row_dynamic_s(ctx, 5, 0);
    return result;
}

enum nk_dialog_result nk_dialog_begin(struct nk_context *ctx, int container_width, int container_height, const char *title,
                                      const char *message, const char *positive, const char *negative, const char *neutral)
{
    struct nk_borders dec_size = nk_style_window_get_decoration_size(&ctx->style, NK_WINDOW_TITLE | NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR);
    int dialog_width = 350 * NK_UI_SCALE, message_width = dialog_width - dec_size.l - dec_size.r;
    // 10 extra dp is to preserve last line, as Nuklear will stop if total line height >= available height
    // Also extra padding can make UI looks better
    int message_height = nk_text_multiline_measure_height(ctx, message_width, message, strlen(message)) + 10 * NK_UI_SCALE;
    int dialog_height = dec_size.t + message_height +
                        ctx->style.window.spacing.y + 30 * NK_UI_SCALE +
                        ctx->style.window.spacing.y + 5 * NK_UI_SCALE +
                        dec_size.b;

    struct nk_rect s = nk_rect_centered(container_width, container_height, dialog_width, dialog_height);
    enum nk_dialog_result result = NK_DIALOG_NONE;
    if (nk_begin(ctx, title, s, NK_WINDOW_TITLE | NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR))
    {
        result = _nk_dialog_content(ctx, message, message_height, positive, negative, neutral);
    }
    return result;
}

enum nk_dialog_result nk_dialog_popup_begin(struct nk_context *ctx, const char *title,
                                            const char *message, const char *positive, const char *negative, const char *neutral)
{
    struct nk_borders dec_size = nk_style_popup_get_decoration_size(&ctx->style, NK_WINDOW_TITLE | NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR);
    int dialog_width = 350 * NK_UI_SCALE, message_width = dialog_width - dec_size.l - dec_size.r;
    // 10 extra dp is to preserve last line, as Nuklear will stop if total line height >= available height
    // Also extra padding can make UI looks better
    int message_height = nk_text_multiline_measure_height(ctx, message_width, message, strlen(message)) + 10 * NK_UI_SCALE;
    int dialog_height = dec_size.t + message_height +
                        ctx->style.window.spacing.y + 30 * NK_UI_SCALE +
                        ctx->style.window.spacing.y + 5 * NK_UI_SCALE +
                        dec_size.b;

    struct nk_vec2 window_size = nk_window_get_size(ctx);
    struct nk_rect s = nk_rect_centered(window_size.x - ctx->style.window.padding.x * 2, window_size.y, dialog_width, dialog_height);
    enum nk_dialog_result result = NK_DIALOG_NONE;
    if (nk_popup_begin(ctx, NK_POPUP_STATIC, title, NK_WINDOW_TITLE | NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR, s))
    {
        result = _nk_dialog_content(ctx, message, message_height, positive, negative, neutral);
    }
    return result;
}

#endif