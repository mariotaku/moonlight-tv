#pragma once
#include "demo/sdl_opengles2/nuklear_sdl_gles2.h"

#define nk_platform_init(appctx) nk_sdl_init((SDL_Window *)appctx)

#define nk_platform_shutdown() nk_sdl_shutdown()

#define nk_platform_font_stash_begin nk_sdl_font_stash_begin

#define nk_platform_font_stash_end nk_sdl_font_stash_end

#ifdef NK_SDL_GLES2_IMPLEMENTATION
static void nk_platform_gl_setup()
{
    /* SDL setup */
    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");
    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
}
#endif