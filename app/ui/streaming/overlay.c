#include <ui/manager.h>
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
#include "streaming.controller.h"

struct nk_vec2 _btn_keyboard_center = {0, 0}, _btn_suspend_center = {0, 0},
        _btn_quit_center = {0, 0}, _btn_confirm_center = {0, 0};
struct nk_dialog_widget_bounds _dialog_bounds;

static void _streaming_error_window(struct nk_context *ctx);

static void _streaming_perf_stat(struct nk_context *ctx);

void _streaming_bottom_bar(struct nk_context *ctx);

bool stream_overlay_showing;

#if TARGET_RASPI
#define OVERLAY_WINDOW_FLAGS NK_WINDOW_TITLE | NK_WINDOW_NO_SCROLLBAR
#else
#define OVERLAY_WINDOW_FLAGS NK_WINDOW_TITLE | NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_MINIMIZABLE | NK_WINDOW_MOVABLE
#endif

void streaming_overlay_init(struct nk_context *ctx) {
    stream_overlay_showing = false;
}

bool streaming_overlay_dispatch_userevent(int which) {
    switch (which) {
        case USER_ST_QUITAPP_CONFIRM:
            streaming_overlay_show();
            return true;
        default:
            break;
    }
    return false;
}

bool streaming_overlay_should_block_input() {
    return stream_overlay_showing;
}

bool streaming_overlay_hide() {
    if (!stream_overlay_showing)
        return false;
    stream_overlay_showing = false;
    streaming_enter_fullscreen();
    return true;
}

bool streaming_overlay_show() {
    if (stream_overlay_showing)
        return false;
    stream_overlay_showing = true;
    struct nk_vec2 wndpos = nk_vec2_s(10, 10);

    streaming_enter_overlay(ui_display_width / 2 - wndpos.x, wndpos.y, ui_display_width / 2, ui_display_height / 2);
    return true;
}

void _streaming_error_window(struct nk_context *ctx) {
    char *message = streaming_errmsg[0] ? streaming_errmsg : (char *) MSG_GS_ERRNO[-streaming_errno];
    enum nk_dialog_result result = nk_dialog_begin(ctx, ui_display_width, ui_display_height, "Streaming Error",
                                                   message, "OK", NULL, NULL, &_dialog_bounds);
    _btn_confirm_center = nk_rect_center(_dialog_bounds.positive);
    if (result != NK_DIALOG_RUNNING) {
        streaming_status = STREAMING_NONE;
    }
    nk_end(ctx);
}

void _streaming_perf_stat(struct nk_context *ctx) {
    struct nk_vec2 wndpos = nk_vec2_s(10, 10);
    struct nk_vec2 wndsize = nk_vec2_s(240, 174);
    struct VIDEO_STATS *dst = &vdec_summary_stats;
    static char title[64];
    snprintf(title, sizeof(title) - 1, "Performance Stats | %.2f FPS", dst->decodedFps);
    if (nk_begin_titled(ctx, "overlay_pref_stats", title, nk_recta(wndpos, wndsize), OVERLAY_WINDOW_FLAGS)) {
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

        if (dst->decodedFrames) {
            nk_label(ctx, "Network frame drop", NK_TEXT_LEFT);
            nk_labelf(ctx, NK_TEXT_RIGHT, "%.2f%%", (float) dst->networkDroppedFrames / dst->totalFrames * 100);
            nk_layout_row_s(ctx, NK_DYNAMIC, 20, 2, ratios73);
            nk_label(ctx, "Decode time", NK_TEXT_LEFT);
            nk_labelf(ctx, NK_TEXT_RIGHT, " %.2f ms", (float) dst->totalDecodeTime / dst->decodedFrames);
        }
    }
    wndpos = nk_window_get_position(ctx);
    nk_end(ctx);
}

lv_obj_t *streaming_scene_create(streaming_controller_t *controller, lv_obj_t *parent) {
    lv_obj_t *scene = lv_obj_create(parent);
    lv_obj_set_style_radius(scene, 0, 0);
    lv_obj_set_style_border_side(scene, LV_BORDER_SIDE_NONE, 0);
    lv_obj_set_style_bg_opa(scene, 0, 0);
    lv_obj_set_size(scene, LV_PCT(100), LV_PCT(100));
    lv_obj_t *progress = lv_spinner_create(scene, 1000, 60);
    lv_obj_set_size(progress, lv_dpx(50), lv_dpx(50));
    lv_obj_center(progress);

    controller->progress = progress;
    return scene;
}