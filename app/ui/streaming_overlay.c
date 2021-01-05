#include "streaming_overlay.h"
#include "messages.h"

#include "stream/session.h"
#include "sdl/user_event.h"

static void _connection_window(struct nk_context *ctx, STREAMING_STATUS stat);
static void _streaming_error_window(struct nk_context *ctx);

bool quit_confirm_showing;

void streaming_overlay_init(struct nk_context *ctx)
{
}

bool streaming_overlay(struct nk_context *ctx, STREAMING_STATUS stat)
{
    switch (stat)
    {
    case STREAMING_CONNECTING:
    case STREAMING_DISCONNECTING:
        /* code */
        _connection_window(ctx, stat);
        break;
    case STREAMING_ERROR:
        _streaming_error_window(ctx);
        break;
    case STREAMING_STREAMING:
        if (quit_confirm_showing)
        {
            _streaming_quit_confirm_window(ctx);
        }
        break;
    default:
        break;
    }
    return true;
}

bool streaming_dispatch_userevent(struct nk_context *ctx, SDL_Event ev)
{
    switch (ev.user.code)
    {
    case SDL_USER_ST_QUITAPP_CONFIRM:
        quit_confirm_showing = true;
        break;
    default:
        break;
    }
}

void _connection_window(struct nk_context *ctx, STREAMING_STATUS stat)
{
    static struct nk_rect s = {330, 240, 300, 60};
    if (nk_begin(ctx, "Connection", s, NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR))
    {
        struct nk_vec2 win_region_size;
        int content_height_remaining;
        win_region_size = nk_window_get_content_region_size(ctx);
        content_height_remaining = (int)win_region_size.y;
        content_height_remaining -= ctx->style.window.padding.y * 2;
        content_height_remaining -= (int)ctx->style.window.border;
        nk_layout_row_dynamic(ctx, content_height_remaining, 1);

        switch (stat)
        {
        case STREAMING_CONNECTING:
            nk_label(ctx, "Connecting...", NK_TEXT_ALIGN_LEFT);
            break;
        case STREAMING_DISCONNECTING:
            nk_label(ctx, "Disconnecting...", NK_TEXT_ALIGN_LEFT);
            break;
        }
    }
    nk_end(ctx);
}

void _streaming_error_window(struct nk_context *ctx)
{
    static struct nk_rect s = {330, 215, 300, 115};
    if (nk_begin(ctx, "Streaming Error", s, NK_WINDOW_TITLE | NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR))
    {
        struct nk_vec2 win_region_size = nk_window_get_content_region_size(ctx);
        int content_height_remaining = (int)win_region_size.y;
        content_height_remaining -= 8 * 2;
        /* remove bottom button height */
        content_height_remaining -= 30;
        nk_layout_row_dynamic(ctx, 40, 1);
        nk_label_wrap(ctx, MSG_GS_ERRNO[-streaming_errno]);
        nk_layout_space_begin(ctx, NK_STATIC, 30, 1);
        nk_layout_space_push(ctx, nk_recti(win_region_size.x - 80, 0, 80, 30));
        if (nk_button_label(ctx, "OK"))
        {
            streaming_status = STREAMING_NONE;
        }
        nk_end(ctx);
    }
}

void _streaming_quit_confirm_window(struct nk_context *ctx)
{
    static struct nk_rect s = {330, 215, 300, 115};
    if (nk_begin(ctx, "Quit Streaming", s, NK_WINDOW_TITLE | NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR))
    {
        struct nk_vec2 win_region_size = nk_window_get_content_region_size(ctx);
        int content_height_remaining = (int)win_region_size.y;
        content_height_remaining -= 8 * 2;
        /* remove bottom button height */
        content_height_remaining -= 30;
        nk_layout_row_dynamic(ctx, 40, 1);
        nk_label_wrap(ctx, "Do you want to quit streaming? If you select Quit Game, unsaved progress will be lost.");
        nk_layout_row_template_begin(ctx, 30);
        nk_layout_row_template_push_static(ctx, 80);
        nk_layout_row_template_push_variable(ctx, 10);
        nk_layout_row_template_push_static(ctx, 80);
        nk_layout_row_template_push_static(ctx, 80);
        nk_layout_row_template_end(ctx);

        if (nk_button_label(ctx, "Cancel"))
        {
            quit_confirm_showing = false;
        }
        nk_spacing(ctx, 1);
        if (nk_button_label(ctx, "Keep Game"))
        {
            streaming_interrupt(false);
            quit_confirm_showing = false;
        }
        if (nk_button_label(ctx, "Quit Game"))
        {
            streaming_interrupt(true);
            quit_confirm_showing = false;
        }
        nk_end(ctx);
    }
}
