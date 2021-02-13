#if OS_WEBOS
#include <GLES2/gl2.h>
#elif OS_DARWIN
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include "root.h"
#include "config.h"

#include "stream/session.h"

#include "launcher/window.h"
#include "settings/window.h"
#include "streaming/overlay.h"

#include "util/bus.h"
#include "util/user_event.h"

#include "res.h"

#if TARGET_DESKTOP || TARGET_RASPI
#include "sdl_renderer.h"
#endif

short ui_display_width, ui_display_height;
short ui_logic_width, ui_logic_height;

bool ui_settings_showing;
bool ui_fake_mouse_click_started;
enum UI_INPUT_MODE ui_input_mode;
struct nk_vec2 ui_statbar_icon_padding;

static bool ui_send_faketouch_cancel;
static bool ui_fake_mouse_event_received;
static struct
{
    struct nk_vec2 *center;
    void *down;
} ui_pending_faketouch;

void ui_root_init(struct nk_context *ctx)
{
    launcher_window_init(ctx);
    settings_window_init(ctx);
    streaming_overlay_init(ctx);
    ui_send_faketouch_cancel = false;
    ui_fake_mouse_click_started = false;
    ui_input_mode = UI_INPUT_MODE_POINTER;
    ui_statbar_icon_padding = nk_vec2_s(2, 2);
}

void ui_root_destroy()
{
    launcher_window_destroy();
}

bool ui_root(struct nk_context *ctx)
{
    if (ui_fake_mouse_event_received && ui_pending_faketouch.center)
    {
        bus_pushevent(USER_FAKEINPUT_MOUSE_CLICK, ui_pending_faketouch.center, (void *)ui_pending_faketouch.down);
    }
    else if (ui_send_faketouch_cancel)
    {
        bus_pushevent(USER_FAKEINPUT_MOUSE_CANCEL, NULL, NULL);
        ui_send_faketouch_cancel = false;
    }
    ui_fake_mouse_event_received = false;
    ui_pending_faketouch.center = NULL;
    STREAMING_STATUS stat = streaming_status;
    if (stat == STREAMING_NONE)
    {
        if (!launcher_window(ctx))
        {
            return false;
        }
        
        if (ui_settings_showing)
        {
            if (!settings_window(ctx))
            {
                settings_window_close();
            }
        }

        return true;
    }
    else
    {
        return streaming_overlay(ctx, stat);
    }
}

void ui_render_background()
{
#if OS_WEBOS
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#else
    if (streaming_status == STREAMING_STREAMING)
    {
        renderer_draw();
    }
    else
    {
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
    }
#endif
}

bool ui_dispatch_userevent(struct nk_context *ctx, int which, void *data1, void *data2)
{
    bool handled = false;
    handled |= launcher_window_dispatch_userevent(which, data1, data2);
    handled |= streaming_overlay_dispatch_userevent(which);
    if (!handled)
    {
        switch (which)
        {
        case USER_FAKEINPUT_MOUSE_MOTION:
        {
            struct nk_vec2 *center = data1;
            nk_input_motion(ctx, center->x, center->y);
            handled = true;
            break;
        }
        case USER_FAKEINPUT_MOUSE_CLICK:
        {
            struct nk_vec2 *center = data1;
            if (ui_fake_mouse_event_received)
            {
                // This is not the first time event received
                ui_pending_faketouch.center = center;
                ui_pending_faketouch.down = data2;
                return true;
            }
            ui_fake_mouse_event_received = true;
            ui_fake_mouse_click_started = true;
            nk_input_motion(ctx, center->x, center->y);
            nk_input_button(ctx, NK_BUTTON_LEFT, center->x, center->y, data2 ? nk_true : nk_false);
            if (!data2)
            {
                ui_send_faketouch_cancel = true;
                ui_fake_mouse_click_started = false;
            }
            return true;
        }
        case USER_FAKEINPUT_MOUSE_CANCEL:
        {
            nk_input_motion(ctx, 0, 0);
            return true;
        }
#if TARGET_DESKTOP
        case USER_STREAM_OPEN:
        {
            PSTREAM_CONFIGURATION conf = data1;
            renderer_setup(conf->width, conf->height);
            return true;
        }
        case USER_STREAM_CLOSE:
            renderer_cleanup();
            break;
        case USER_SDL_FRAME:
            renderer_submit_frame(data1, data2);
            return true;
#endif
        default:
            break;
        }
    }
    return handled;
}

bool ui_should_block_input()
{
    bool ret = false;
    ret |= streaming_overlay_should_block_input();
    return ret;
}

void ui_display_size(struct nk_context *ctx, short width, short height)
{
    ui_display_width = width;
    ui_display_height = height;
    ui_logic_width = width / NK_UI_SCALE;
    ui_logic_height = height / NK_UI_SCALE;
}

bool ui_dispatch_navkey(struct nk_context *ctx, NAVKEY key, bool down, uint32_t timestamp)
{
    bool handled = false;
    if (streaming_status == STREAMING_NONE)
    {
        handled |= handled || (!down && ui_settings_showing && settings_window_dispatch_navkey(ctx, key));
        handled |= handled || launcher_window_dispatch_navkey(ctx, key, down, timestamp);
    }
    else
    {
        handled |= handled || streaming_overlay_dispatch_navkey(ctx, key, down);
    }
    return handled;
}

bool ui_set_input_mode(enum UI_INPUT_MODE mode)
{
    if (ui_input_mode == mode)
    {
        return false;
    }
    ui_input_mode = mode;
    return true;
}
