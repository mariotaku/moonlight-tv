#include "gui_root.h"

#include <GLES2/gl2.h>

#include "stream/session.h"

#include "launcher_window.h"
#include "streaming_overlay.h"

short gui_display_width, gui_display_height;

void gui_root_init(struct nk_context *ctx)
{
    launcher_window_init(ctx);
    streaming_overlay_init(ctx);
}

bool gui_root(struct nk_context *ctx)
{
    STREAMING_STATUS stat = streaming_status;
    if (stat == STREAMING_NONE)
    {
        return launcher_window(ctx);
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

bool gui_dispatch_userevent(struct nk_context *ctx, SDL_Event ev)
{
    return false;
}

bool gui_dispatch_inputevent(struct nk_context *ctx, SDL_Event ev)
{
    return false;
}

void gui_display_size(short width, short height)
{
    gui_display_width = width;
    gui_display_height = height;
}