#pragma once
#include "demo/sdl_opengl2/nuklear_sdl_gl2.h"

#define nk_platform_init(appctx) nk_sdl_init((SDL_Window *)appctx)

#define nk_platform_shutdown() nk_sdl_shutdown()

#define nk_platform_font_stash_begin nk_sdl_font_stash_begin

#define nk_platform_font_stash_end nk_sdl_font_stash_end

#ifdef NK_SDL_GL2_IMPLEMENTATION
static void nk_platform_gl_setup()
{
    /* SDL setup */
    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");
    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
}
#endif