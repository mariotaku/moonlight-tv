#include "gui_root.h"

#include <GLES2/gl2.h>

#include "backend/streaming_session.h"

#include "computers_window.h"
#include "applications_window.h"
#include "streaming_overlay.h"

void gui_root_init(struct nk_context *ctx)
{
    computers_window_init(ctx);
    applications_window_init(ctx);
    streaming_overlay_init(ctx);
}

bool gui_root(struct nk_context *ctx)
{
    STREAMING_STATUS stat = streaming_status();
    if (stat == STREAMING_NONE)
    {
        return computers_window(ctx);
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

bool gui_dispatch_event(SDL_Event ev)
{
    return false;
}