#if HAVE_GLES2

#include <GLES2/gl2.h>

#elif OS_DARWIN
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include "app.h"
#include "root.h"
#include "config.h"

#include "stream/platform.h"
#include "stream/session.h"

#include "launcher/window.h"
#include "settings/settings.controller.h"
#include "streaming/overlay.h"

#include "util/bus.h"
#include "util/user_event.h"

#include "res.h"

short ui_display_width, ui_display_height;
short ui_logic_width, ui_logic_height;
float ui_scale = 1;

bool ui_settings_showing;
bool ui_fake_mouse_click_started;
enum UI_INPUT_MODE ui_input_mode;
struct nk_vec2 ui_statbar_icon_padding;

static PVIDEO_RENDER_CALLBACKS ui_stream_render;
static bool ui_send_faketouch_cancel;
static bool ui_fake_mouse_event_received;

void ui_root_init(struct nk_context *ctx) {
    streaming_overlay_init(ctx);
    ui_send_faketouch_cancel = false;
    ui_fake_mouse_click_started = false;
    ui_input_mode = UI_INPUT_MODE_POINTER;
    ui_statbar_icon_padding = nk_vec2_s(2, 2);
}

bool ui_dispatch_userevent(int which, void *data1, void *data2) {
    bool handled = false;
    handled |= uimanager_dispatch_event(app_uimanager, which, data1, data2);
    if (!handled) {
        switch (which) {
            case USER_STREAM_OPEN:
                ui_stream_render = decoder_get_render(decoder_current);
                if (ui_stream_render) {
                    app_force_redraw = true;
                    ui_stream_render->renderSetup((PSTREAM_CONFIGURATION) data1, app_render_queue_submit);
                }
                app_set_keep_awake(true);
                streaming_enter_fullscreen();
                return true;
            case USER_STREAM_CLOSE:
                if (ui_stream_render) {
                    ui_stream_render->renderCleanup();
                    app_force_redraw = false;
                }
                app_set_keep_awake(false);
                app_set_mouse_grab(false);
                ui_stream_render = NULL;
                return true;
            case USER_SDL_FRAME:
                if (ui_stream_render)
                    ui_stream_render->renderSubmit(data1);
                return true;
            default:
                break;
        }
    }
    return handled;
}

bool ui_should_block_input() {
    bool ret = false;
    ret |= streaming_overlay_should_block_input();
    return ret;
}

void ui_display_size(short width, short height) {
    ui_display_width = width;
    ui_display_height = height;
    ui_scale = width / 640.0;
    ui_logic_width = width / ui_scale;
    ui_logic_height = height / ui_scale;
    applog_i("UI", "Display size changed to %d x %d", width, height);
    nk_ext_sprites_init();
}

bool ui_set_input_mode(enum UI_INPUT_MODE mode) {
    if (ui_input_mode == mode) {
        return false;
    }
    ui_input_mode = mode;
    return true;
}
