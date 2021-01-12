#ifdef OS_DARWIN
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include "gui_root.h"
#include "ui/config.h"

#include "stream/session.h"

#include "launcher_window.h"
#include "settings_window.h"
#include "streaming_overlay.h"

short gui_display_width, gui_display_height;
short gui_logic_width, gui_logic_height;

bool gui_settings_showing;

void gui_root_init(struct nk_context *ctx)
{
    launcher_window_init(ctx);
    settings_window_init(ctx);
    streaming_overlay_init(ctx);
}

bool gui_root(struct nk_context *ctx)
{
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

bool gui_dispatch_userevent(int which)
{
    bool handled = false;
    handled |= streaming_overlay_dispatch_userevent(which);
    return false;
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

#ifdef HAVE_SDL
bool gui_dispatch_inputevent(struct nk_context *ctx, SDL_Event ev)
{
    if (ev.type == SDL_KEYUP)
    {
        printf("SDL_KEYUP scancode: %s, sym: %s\n", SDL_GetScancodeName(ev.key.keysym.scancode), SDL_GetKeyName(ev.key.keysym.sym));
    }
    return false;
}
#endif