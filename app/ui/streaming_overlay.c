#include "streaming_overlay.h"
#include "gui_root.h"
#include "messages.h"

#include "stream/session.h"
#include "util/user_event.h"

static void _connection_window(struct nk_context *ctx, STREAMING_STATUS stat);
static void _streaming_error_window(struct nk_context *ctx);
static void _streaming_quit_confirm_window(struct nk_context *ctx);

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

bool streaming_overlay_dispatch_userevent(int which)
{
    switch (which)
    {
    case USER_ST_QUITAPP_CONFIRM:
        quit_confirm_showing = true;
        break;
    default:
        break;
    }
    return false;
}

bool streaming_overlay_dispatch_navkey(struct nk_context *ctx, NAVKEY navkey)
{
    if (quit_confirm_showing)
    {
        switch (navkey)
        {
        case NAVKEY_BACK:
            quit_confirm_showing = false;
            break;
        case NAVKEY_CONFIRM:
            streaming_interrupt(true);
            quit_confirm_showing = false;
            break;
        case NAVKEY_ALTERNATIVE:
            streaming_interrupt(false);
            quit_confirm_showing = false;
            break;
        default:
            break;
        }
        return true;
    }
    return false;
}

bool streaming_overlay_should_block_input()
{
    return quit_confirm_showing;
}

void _connection_window(struct nk_context *ctx, STREAMING_STATUS stat)
{
    struct nk_rect s = nk_rect_s_centered(gui_logic_width, gui_logic_height, 330, 60);
    if (nk_begin(ctx, "Connection", s, NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR))
    {
        struct nk_vec2 content_size = nk_window_get_content_inner_size(ctx);
        int content_height_remaining = (int)content_size.y;
        nk_layout_row_dynamic(ctx, content_height_remaining, 1);

        switch (stat)
        {
        case STREAMING_CONNECTING:
            nk_label(ctx, "Connecting...", NK_TEXT_ALIGN_LEFT);
            break;
        case STREAMING_DISCONNECTING:
            nk_label(ctx, "Disconnecting...", NK_TEXT_ALIGN_LEFT);
            break;
        default:
            break;
        }
    }
    nk_end(ctx);
}

void _streaming_error_window(struct nk_context *ctx)
{
    char *message = streaming_errmsg[0] ? streaming_errmsg : (char *)MSG_GS_ERRNO[-streaming_errno];
    enum nk_dialog_result result = nk_dialog_begin(ctx, gui_display_width, gui_display_height, "Streaming Error",
                                                   message, "OK", NULL, NULL);
    if (result != NK_DIALOG_RUNNING)
    {
        streaming_status = STREAMING_NONE;
    }
    nk_end(ctx);
}

void _streaming_quit_confirm_window(struct nk_context *ctx)
{
    const char *message = "Do you want to quit streaming? "
                          "If you select \"Quit\", unsaved progress will be lost.";
    enum nk_dialog_result result = nk_dialog_begin(ctx, gui_display_width, gui_display_height, "Quit Streaming",
                                                   message, "Quit", "Keep", "Cancel");
    switch (result)
    {
    case NK_DIALOG_POSITIVE:
        streaming_interrupt(true);
        quit_confirm_showing = false;
        break;
    case NK_DIALOG_NEGATIVE:
        streaming_interrupt(false);
        quit_confirm_showing = false;
        break;
    case NK_DIALOG_NEUTRAL:
        quit_confirm_showing = false;
        break;
    default:
        break;
    }
    nk_end(ctx);
}
