#include <GLES2/gl2.h>

#include "application_root.h"
#include "computers_window.h"

void application_root(struct nk_context *ctx)
{
    computers_window(ctx);
}

void application_background()
{
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}