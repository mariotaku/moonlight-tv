#include <GLES2/gl2.h>

#include "gui_root.h"
#include "computers_window.h"
#include "applications_window.h"

void gui_root_init(struct nk_context *ctx)
{
    computers_window_init(ctx);
    applications_window_init(ctx);
}

bool gui_root(struct nk_context *ctx)
{
    return computers_window(ctx);
}

void gui_background()
{
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}