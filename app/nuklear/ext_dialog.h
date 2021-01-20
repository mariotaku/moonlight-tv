#pragma once
#ifndef NK_NUKLEAR_H_
#include "nuklear.h"
#include "ext_functions.h"
#endif

#ifndef NK_UI_SCALE
#warning "NK_UI_SCALE not defined"
#define NK_UI_SCALE 1
#endif

enum nk_dialog_result
{
    NK_DIALOG_CANCELLED = -1,
    NK_DIALOG_RUNNING,
    NK_DIALOG_POSITIVE,
    NK_DIALOG_NEGATIVE,
    NK_DIALOG_NEUTRAL
};

enum nk_dialog_result nk_dialog(struct nk_context *ctx, int container_width, int container_height, const char *title,
                                const char *message, const char *positive, const char *negative, const char *neutral);

#ifdef NK_IMPLEMENTATION

enum nk_dialog_result nk_dialog(struct nk_context *ctx, int container_width, int container_height, const char *title,
                                const char *message, const char *positive, const char *negative, const char *neutral)
{
    struct nk_rect s = nk_rect_centered(container_width, container_height, 330 * NK_UI_SCALE, 150 * NK_UI_SCALE);
    enum nk_dialog_result result = NK_DIALOG_RUNNING;
    if (nk_begin(ctx, title, s, NK_WINDOW_TITLE | NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR))
    {
        struct nk_vec2 content_size = nk_window_get_content_inner_size(ctx);
        int content_height_remaining = (int)content_size.y;
        /* remove bottom button height */
        content_height_remaining -= 30 * NK_UI_SCALE;
        content_height_remaining -= ctx->style.window.spacing.y;
        nk_layout_row_dynamic(ctx, content_height_remaining, 1);
        nk_label_wrap(ctx, message);

        nk_layout_row_template_begin_s(ctx, 30);
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
        nk_layout_row_template_end(ctx);

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
        nk_end(ctx);
    }
    return result;
}

#endif