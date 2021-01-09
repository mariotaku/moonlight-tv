#include "gui_root.h"

#include <GLES2/gl2.h>

#include "stream/session.h"

#include "launcher_window.h"
#include "settings_window.h"
#include "streaming_overlay.h"

short gui_display_width, gui_display_height;

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

bool gui_block_stream_inputevent()
{
    bool ret = false;
    ret |= streaming_overlay_block_stream_inputevent();
    return ret;
}

void gui_display_size(short width, short height)
{
    gui_display_width = width;
    gui_display_height = height;
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