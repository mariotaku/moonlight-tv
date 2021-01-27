#include "res.h"

#include <stddef.h>

#include "sdl_renderer.h"
#include "gui_root.h"

#define GL_GLEXT_PROTOTYPES

#include <SDL_opengl.h>
#include <SDL_opengl_glext.h>

static int width, height;

void renderer_setup(int w, int h)
{
    width = w;
    height = h;
}

void renderer_submit_frame(void *data1, void *data2)
{
    uint8_t **image = data1;
}

void renderer_draw()
{
}

void renderer_cleanup()
{
}