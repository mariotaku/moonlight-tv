#include "app.h"
#include "root.h"
#include "res.h"

#include "lvgl/lv_disp_drv_app.h"
#include "draw/sdl/lv_draw_sdl_utils.h"

#include "stream/platform.h"
#include "stream/session.h"
#include "stream/input/absinput.h"

#include "streaming/streaming.controller.h"
#include "launcher/launcher.controller.h"

#include "util/bus.h"
#include "util/user_event.h"
#include "util/logging.h"

short ui_display_width, ui_display_height;
static unsigned int last_pts = 0;

enum UI_INPUT_MODE ui_input_mode;
lv_img_dsc_t ui_logo_src_ = {.data_size = 0};

typedef struct render_frame_req_t {
    void *data;
    unsigned int pts;
} render_frame_req_t;

static PVIDEO_RENDER_CALLBACKS ui_stream_render;
static HOST_RENDER_CONTEXT ui_stream_render_host_context = {
        .queueSubmit = ui_render_queue_submit
};

bool ui_has_stream_renderer() {
    return ui_stream_render != NULL && ui_stream_render->renderDraw;
}

bool ui_render_background() {
    if (streaming_status == STREAMING_STREAMING && ui_stream_render && ui_stream_render->renderDraw) {
        return ui_stream_render->renderDraw();
    }
    return false;
}

bool ui_dispatch_userevent(int which, void *data1, void *data2) {
    bool handled = false;
    ui_userevent_t userdata = {data1, data2};
    handled |= lv_fragment_manager_send_event(app_uimanager, which, &userdata);
    if (!handled) {
        switch (which) {
            case USER_STREAM_OPEN: {
                lv_draw_sdl_drv_param_t *param = lv_disp_get_default()->driver->user_data;
                ui_stream_render_host_context.renderer = param->renderer;
                ui_stream_render = decoder_get_render(decoder_current);
                if (ui_stream_render) {
                    ui_stream_render->renderSetup((PSTREAM_CONFIGURATION) data1, &ui_stream_render_host_context);
                }
                absinput_start();
                app_set_keep_awake(true);
                streaming_enter_fullscreen();
                last_pts = 0;
                return true;
            }
            case USER_STREAM_CLOSE:
                if (ui_stream_render) {
                    ui_stream_render->renderCleanup();
                }
                app_set_keep_awake(false);
                app_set_mouse_grab(false);
                absinput_stop();
                ui_stream_render = NULL;
                ui_stream_render_host_context.renderer = NULL;
                return true;
            default:
                break;
        }
    }
    return handled;
}

bool ui_should_block_input() {
    return streaming_overlay_shown();
}

void ui_display_size(short width, short height) {
    ui_display_width = width;
    ui_display_height = height;
    applog_i("UI", "Display size changed to %d x %d", width, height);
}

bool ui_set_input_mode(enum UI_INPUT_MODE mode) {
    if (ui_input_mode == mode) {
        return false;
    }
    ui_input_mode = mode;
    return true;
}

const lv_img_dsc_t *ui_logo_src() {
    if (ui_logo_src_.data_size == 0) {
        ui_logo_src_.header.w = LV_DPX(NAV_LOGO_SIZE);
        ui_logo_src_.header.h = LV_DPX(NAV_LOGO_SIZE);
        ui_logo_src_.header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA;

        ui_logo_src_.data_size = sizeof(lv_sdl_img_data_logo_96);
        ui_logo_src_.data = (const uint8_t *) &lv_sdl_img_data_logo_96;
    }
    return &ui_logo_src_;
}

static void handle_queued_frame(render_frame_req_t *req) {
    bool redraw = false;
    if (last_pts > req->pts) {
        applog_w("Stream", "Ignore late frame pts: %d", req->pts);
        return;
    }
    if (ui_stream_render && ui_stream_render->renderSubmit(req->data)) {
        redraw = true;
    }
    if (redraw) {
        lv_app_redraw_now(lv_disp_get_default()->driver);
    }
}

bool ui_render_queue_submit(void *data, unsigned int pts) {
//    applog_d("Stream", "Submit frame. pts: %d", pts);
    render_frame_req_t req = {.data = data, .pts = pts};
    bus_pushaction_sync((bus_actionfunc) handle_queued_frame, &req);
    return true;
}

void ui_cb_destroy_fragment(lv_event_t *e) {
    lv_fragment_t *fragment = lv_event_get_user_data(e);
    if (fragment->cls->obj_deleted_cb) {
        fragment->cls->obj_deleted_cb(fragment, lv_event_get_target(e));
    }
    fragment->obj = NULL;
    lv_fragment_del(fragment);
}