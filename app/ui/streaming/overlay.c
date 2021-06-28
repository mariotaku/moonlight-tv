#include "overlay.h"
#include "priv.h"
#include "ui/root.h"
#include "ui/messages.h"
#include "ui/fonts.h"

#include "stream/platform.h"
#include "stream/session.h"
#include "stream/video/delegate.h"
#include "util/bus.h"
#include "util/user_event.h"

struct nk_vec2 _btn_keyboard_center = {0, 0}, _btn_suspend_center = {0, 0},
               _btn_quit_center = {0, 0}, _btn_confirm_center = {0, 0};
struct nk_dialog_widget_bounds _dialog_bounds;

static void _connection_window(struct nk_context *ctx, STREAMING_STATUS stat);
static void _streaming_error_window(struct nk_context *ctx);

static void _overlay_backdrop(struct nk_context *ctx);
static void _streaming_perf_stat(struct nk_context *ctx);
void _streaming_bottom_bar(struct nk_context *ctx);

void _overlay_windows_push_style(struct nk_context *ctx);
void _overlay_windows_pop_style(struct nk_context *ctx);

bool stream_overlay_showing;

#if TARGET_RASPI
#define OVERLAY_WINDOW_FLAGS NK_WINDOW_TITLE | NK_WINDOW_NO_SCROLLBAR
#else
#define OVERLAY_WINDOW_FLAGS NK_WINDOW_TITLE | NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_MINIMIZABLE | NK_WINDOW_MOVABLE
#endif

void streaming_overlay_init(struct nk_context *ctx)
{
    stream_overlay_showing = false;
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
        if (!stream_overlay_showing)
            // Nothing Nuklear is showing on screen
            return false;
        _overlay_backdrop(ctx);
        _streaming_bottom_bar(ctx);
        _streaming_perf_stat(ctx);
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
        streaming_overlay_show();
        return true;
    default:
        break;
    }
    return false;
}

bool streaming_overlay_dispatch_navkey(struct nk_context *ctx, NAVKEY navkey, NAVKEY_STATE state)
{
    if (stream_overlay_showing)
    {
        switch (navkey)
        {
        case NAVKEY_CANCEL:
            if (state == NAVKEY_STATE_UP)
            {
                streaming_overlay_hide();
            }
            break;
        case NAVKEY_NEGATIVE:
            bus_pushevent(USER_FAKEINPUT_MOUSE_CLICK, &_btn_quit_center, (void *)state);
            break;
        case NAVKEY_MENU:
            bus_pushevent(USER_FAKEINPUT_MOUSE_CLICK, &_btn_suspend_center, (void *)state);
            break;
        case NAVKEY_ALTERNATIVE:
            bus_pushevent(USER_FAKEINPUT_MOUSE_CLICK, &_btn_keyboard_center, (void *)state);
            break;
        default:
            break;
        }
        return true;
    }
    else if (streaming_status == STREAMING_ERROR)
    {
        switch (navkey)
        {
        case NAVKEY_CANCEL:
        case NAVKEY_NEGATIVE:
        case NAVKEY_CONFIRM:
            bus_pushevent(USER_FAKEINPUT_MOUSE_CLICK, &_btn_confirm_center, (void *)state);
            return true;
        default:
            break;
        }
        return false;
    }
    return false;
}

bool streaming_overlay_should_block_input()
{
    return stream_overlay_showing;
}

bool streaming_overlay_hide()
{
    if (!stream_overlay_showing)
        return false;
    stream_overlay_showing = false;
    streaming_enter_fullscreen();
    return true;
}

bool streaming_overlay_show()
{
    if (stream_overlay_showing)
        return false;
    stream_overlay_showing = true;
    struct nk_vec2 wndpos = nk_vec2_s(10, 10);

    streaming_enter_overlay(ui_display_width / 2 - wndpos.x, wndpos.y, ui_display_width / 2, ui_display_height / 2);
    return true;
}

void _connection_window(struct nk_context *ctx, STREAMING_STATUS stat)
{
    struct nk_rect s = nk_rect_s_centered(ui_logic_width, ui_logic_height, 330, 60);
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
    enum nk_dialog_result result = nk_dialog_begin(ctx, ui_display_width, ui_display_height, "Streaming Error",
                                                   message, "OK", NULL, NULL, &_dialog_bounds);
    _btn_confirm_center = nk_rect_center(_dialog_bounds.positive);
    if (result != NK_DIALOG_RUNNING)
    {
        streaming_status = STREAMING_NONE;
    }
    nk_end(ctx);
}

void _overlay_backdrop(struct nk_context *ctx)
{
    if (!nk_window_is_any_hovered(ctx))
    {
        if (nk_input_mouse_clicked(&ctx->input, NK_BUTTON_LEFT, nk_rect(0, 0, ui_display_width, ui_display_height)))
        {
            streaming_overlay_hide();
        }
    }
}

void _streaming_perf_stat(struct nk_context *ctx)
{
    _overlay_windows_push_style(ctx);
    struct nk_vec2 wndpos = nk_vec2_s(10, 10);
    struct nk_vec2 wndsize = nk_vec2_s(240, 174);
    struct VIDEO_STATS *dst = &vdec_summary_stats;
    static char title[64];
    snprintf(title, sizeof(title) - 1, "Performance Stats | %.2f FPS", dst->decodedFps);
    if (nk_begin_titled(ctx, "overlay_pref_stats", title, nk_recta(wndpos, wndsize), OVERLAY_WINDOW_FLAGS))
    {
        static const float ratios73[] = {0.7f, 0.3f};
        static const float ratios46[] = {0.4f, 0.6f};
        nk_layout_row_s(ctx, NK_DYNAMIC, 20, 2, ratios46);
        nk_label(ctx, "Decoder", NK_TEXT_LEFT);
        nk_labelf(ctx, NK_TEXT_RIGHT, "%s (%s)", decoder_definitions[decoder_current].name, vdec_stream_info.format);
        nk_label(ctx, "Audio backend", NK_TEXT_LEFT);
        if (audio_current == AUDIO_DECODER)
            nk_label(ctx, "Use decoder", NK_TEXT_RIGHT);
        else if (audio_current >= 0)
            nk_label(ctx, audio_definitions[audio_current].name, NK_TEXT_RIGHT);
        nk_layout_row_s(ctx, NK_DYNAMIC, 20, 2, ratios46);
        nk_label(ctx, "Network RTT", NK_TEXT_LEFT);
        nk_labelf(ctx, NK_TEXT_RIGHT, "%d ms (var. %d ms)", dst->rtt, dst->rttVariance);
        nk_layout_row_s(ctx, NK_DYNAMIC, 20, 2, ratios73);
        nk_label(ctx, "Network framerate", NK_TEXT_LEFT);
        nk_labelf(ctx, NK_TEXT_RIGHT, "%.2f FPS", dst->receivedFps);

        if (dst->decodedFrames)
        {
            nk_label(ctx, "Network frame drop", NK_TEXT_LEFT);
            nk_labelf(ctx, NK_TEXT_RIGHT, "%.2f%%", (float)dst->networkDroppedFrames / dst->totalFrames * 100);
            nk_layout_row_s(ctx, NK_DYNAMIC, 20, 2, ratios73);
            nk_label(ctx, "Decode time", NK_TEXT_LEFT);
            nk_labelf(ctx, NK_TEXT_RIGHT, " %.2f ms", (float)dst->totalDecodeTime / dst->decodedFrames);
        }
    }
    wndpos = nk_window_get_position(ctx);
    nk_end(ctx);
    _overlay_windows_pop_style(ctx);
}

void _overlay_windows_push_style(struct nk_context *ctx)
{

    struct nk_style_item window_bg = ctx->style.window.fixed_background,
                         header_bg = ctx->style.window.header.normal,
                         button_bg = ctx->style.window.header.close_button.normal;
    window_bg.data.color.a = 160;
    header_bg.data.color.a = 192;
    button_bg.data.color.a = 64;
    nk_style_push_style_item(ctx, &ctx->style.window.fixed_background, window_bg);
    nk_style_push_style_item(ctx, &ctx->style.window.header.normal, header_bg);
    nk_style_push_style_item(ctx, &ctx->style.window.header.active, header_bg);
    nk_style_push_style_item(ctx, &ctx->style.window.header.hover, header_bg);
    nk_style_push_style_item(ctx, &ctx->style.window.header.minimize_button.normal, button_bg);
    nk_style_push_style_item(ctx, &ctx->style.window.header.close_button.normal, button_bg);
    nk_style_push_vec2(ctx, &ctx->style.window.header.label_padding, nk_vec2_s(2, 2));
    nk_style_push_vec2(ctx, &ctx->style.window.header.padding, nk_vec2_s(2, 2));
    nk_style_push_font(ctx, &font_ui_15->handle);
}

void _overlay_windows_pop_style(struct nk_context *ctx)
{
    nk_style_pop_font(ctx);
    nk_style_pop_vec2(ctx);
    nk_style_pop_vec2(ctx);
    nk_style_pop_style_item(ctx);
    nk_style_pop_style_item(ctx);
    nk_style_pop_style_item(ctx);
    nk_style_pop_style_item(ctx);
    nk_style_pop_style_item(ctx);
    nk_style_pop_style_item(ctx);
}