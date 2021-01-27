#include "streaming_overlay.h"
#include "gui_root.h"
#include "messages.h"

#include "stream/session.h"
#include "stream/video/delegate.h"
#include "util/bus.h"
#include "util/user_event.h"

static struct nk_vec2 _btn_suspend_center = {0, 0}, _btn_quit_center = {0, 0};

static void _connection_window(struct nk_context *ctx, STREAMING_STATUS stat);
static void _streaming_error_window(struct nk_context *ctx);
static void _streaming_perf_stat(struct nk_context *ctx);
static void _streaming_quit_confirm_window(struct nk_context *ctx);
static void _streaming_bottom_bar(struct nk_context *ctx);

bool stream_overlay_showing;

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
        if (stream_overlay_showing)
        {
            _streaming_perf_stat(ctx);
            _streaming_bottom_bar(ctx);
            // _streaming_quit_confirm_window(ctx);
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
        stream_overlay_showing = true;
        break;
    default:
        break;
    }
    return false;
}

bool streaming_overlay_dispatch_navkey(struct nk_context *ctx, NAVKEY navkey, bool down)
{
    if (stream_overlay_showing)
    {
        switch (navkey)
        {
        case NAVKEY_CANCEL:
            stream_overlay_showing = false;
            break;
        case NAVKEY_CONFIRM:
            bus_pushevent(USER_FAKEINPUT_MOUSE_CLICK, &_btn_quit_center, (void *)down);
            break;
        case NAVKEY_ALTERNATIVE:
            bus_pushevent(USER_FAKEINPUT_MOUSE_CLICK, &_btn_suspend_center, (void *)down);
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
    return stream_overlay_showing;
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
        stream_overlay_showing = false;
        break;
    case NK_DIALOG_NEGATIVE:
        streaming_interrupt(false);
        stream_overlay_showing = false;
        break;
    case NK_DIALOG_NEUTRAL:
        stream_overlay_showing = false;
        break;
    default:
        break;
    }
    nk_end(ctx);
}

void _streaming_perf_stat(struct nk_context *ctx)
{
    if (nk_begin(ctx, "Stats", nk_rect_s(10, 10, 300, 200), NK_WINDOW_TITLE))
    {
        struct VIDEO_STATS *dst = &vdec_summary_stats;
        nk_layout_row_dynamic_s(ctx, 20, 1);
        nk_labelf(ctx, NK_TEXT_ALIGN_LEFT, "net: %.2f FPS", dst->receivedFps);
        nk_labelf(ctx, NK_TEXT_ALIGN_LEFT, "dec: %.2f FPS", dst->decodedFps);
        if (dst->decodedFrames)
        {
            nk_labelf(ctx, NK_TEXT_ALIGN_LEFT, "net drop: %.2f%%", (float)dst->networkDroppedFrames / dst->totalFrames * 100);
            nk_labelf(ctx, NK_TEXT_ALIGN_LEFT, "avg recv: %.2fms", (float)dst->totalReassemblyTime / dst->receivedFrames);
            nk_labelf(ctx, NK_TEXT_ALIGN_LEFT, "avg submit: %.2fms", (float)dst->totalDecodeTime / dst->decodedFrames);
        }
    }
    nk_end(ctx);
}

void _streaming_bottom_bar(struct nk_context *ctx)
{
    const int bar_height = 50 * NK_UI_SCALE;
    nk_style_push_vec2(ctx, &ctx->style.window.padding, nk_vec2_s(15, 10));
    if (nk_begin(ctx, "Overlay BottomBar", nk_rect(0, gui_display_height - bar_height, gui_display_width, bar_height), NK_WINDOW_NO_SCROLLBAR))
    {
        nk_layout_row_template_begin_s(ctx, 30);
        nk_layout_row_template_push_static_s(ctx, 100);
        nk_layout_row_template_push_variable_s(ctx, 10);
        nk_layout_row_template_push_static_s(ctx, 100);
        nk_layout_row_template_end(ctx);

        struct nk_rect bounds = nk_widget_bounds(ctx);
        _btn_suspend_center.x = nk_rect_center_x(bounds);
        _btn_suspend_center.y = nk_rect_center_y(bounds);
        if (nk_button_label(ctx, "<- Games"))
        {
            streaming_interrupt(false);
            stream_overlay_showing = false;
        }
        nk_spacing(ctx, 1);
        bounds = nk_widget_bounds(ctx);
        _btn_quit_center.x = nk_rect_center_x(bounds);
        _btn_quit_center.y = nk_rect_center_y(bounds);
        if (nk_button_label(ctx, "Quit game"))
        {
            streaming_interrupt(true);
            stream_overlay_showing = false;
        }
    }
    nk_style_pop_vec2(ctx);
    nk_end(ctx);
}