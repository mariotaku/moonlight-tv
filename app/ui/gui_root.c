#if OS_WEBOS
#include <GLES2/gl2.h>
#elif OS_DARWIN
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include "gui_root.h"
#include "ui/config.h"

#include "stream/session.h"

#include "launcher/window.h"
#include "settings_window.h"
#include "streaming_overlay.h"

#include "util/bus.h"
#include "util/user_event.h"

short gui_display_width, gui_display_height;
short gui_logic_width, gui_logic_height;

bool gui_settings_showing;

static bool gui_send_faketouch_cancel;

void gui_root_init(struct nk_context *ctx)
{
    launcher_window_init(ctx);
    settings_window_init(ctx);
    streaming_overlay_init(ctx);
    gui_send_faketouch_cancel = false;
}

void gui_root_destroy()
{
    launcher_window_destroy();
}

bool gui_root(struct nk_context *ctx)
{
    if (gui_send_faketouch_cancel)
    {
        bus_pushevent(USER_FAKEINPUT_MOUSE_CANCEL, NULL, NULL);
        gui_send_faketouch_cancel = false;
    }
    STREAMING_STATUS stat = streaming_status;
    if (stat == STREAMING_NONE)
    {
        if (launcher_window(ctx))
        {
            if (gui_settings_showing)
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
            return false;
        }
    }
    else
    {
        return streaming_overlay(ctx, stat);
    }
}

void gui_background()
{
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

bool gui_dispatch_userevent(struct nk_context *ctx, int which, void *data1, void *data2)
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
            nk_input_motion(ctx, center->x, center->y);
            nk_input_button(ctx, NK_BUTTON_LEFT, center->x, center->y, data2 ? nk_true : nk_false);
            if (!data2)
            {
                gui_send_faketouch_cancel = true;
            }
            break;
        }
        case USER_FAKEINPUT_MOUSE_CANCEL:
        {
            nk_input_motion(ctx, 0, 0);
            break;
        }
        default:
            break;
        }
    }
    return handled;
}

bool gui_should_block_input()
{
    bool ret = false;
    ret |= streaming_overlay_should_block_input();
    return ret;
}

void gui_display_size(struct nk_context *ctx, short width, short height)
{
    gui_display_width = width;
    gui_display_height = height;
    gui_logic_width = width / NK_UI_SCALE;
    gui_logic_height = height / NK_UI_SCALE;
}

bool gui_dispatch_navkey(struct nk_context *ctx, NAVKEY key, bool down)
{
    bool handled = false;
    if (streaming_status == STREAMING_NONE)
    {
        handled |= handled || (!down && gui_settings_showing && settings_window_dispatch_navkey(ctx, key));
        handled |= handled || launcher_window_dispatch_navkey(ctx, key, down);
    }
    else
    {
        handled |= handled || (!down && streaming_overlay_dispatch_navkey(ctx, key));
    }
    return handled;
}