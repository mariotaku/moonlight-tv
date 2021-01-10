#ifndef NK_NUKLEAR_H_
#include "nuklear.h"
#endif

NK_API nk_bool nk_list_item_label(struct nk_context *ctx, const char *label, nk_flags align);
NK_API nk_bool nk_list_item_text(struct nk_context *ctx, const char *text, int len,
                                 nk_flags alignment);

NK_API nk_bool nk_horiz_list_view_begin(struct nk_context *, struct nk_list_view *out,
                                        const char *id, nk_flags, int row_height, int row_count);
NK_API void nk_horiz_list_view_end(struct nk_list_view *);

#ifdef NK_IMPLEMENTATION

#include "horiz_list_view.h"

NK_API nk_bool
nk_list_item_label(struct nk_context *ctx, const char *label, nk_flags align)
{
    return nk_list_item_text(ctx, label, nk_strlen(label), align);
}

NK_API nk_bool
nk_list_item_text(struct nk_context *ctx, const char *text, int len,
                  nk_flags alignment)
{
    struct nk_window *win;
    const struct nk_input *in;
    const struct nk_style *style;

    struct nk_rect bounds;
    enum nk_widget_layout_states state;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    win = ctx->current;
    style = &ctx->style;
    state = nk_widget_fitting(&bounds, ctx, style->contextual_button.padding);
    if (!state)
        return nk_false;

    in = (state == NK_WIDGET_ROM || win->layout->flags & NK_WINDOW_ROM) ? 0 : &ctx->input;
    return nk_do_button_text(&ctx->last_widget_state, &win->buffer, bounds,
                             text, len, alignment, NK_BUTTON_DEFAULT, &style->contextual_button, in, style->font);
}
#endif
