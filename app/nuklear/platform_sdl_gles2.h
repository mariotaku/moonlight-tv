#pragma once
#include "nuklear_sdl_gles2.h"

void nk_platform_preinit();

#define nk_platform_init(appctx) nk_sdl_init((SDL_Window *)appctx)

#define nk_platform_render() nk_sdl_render(NK_ANTI_ALIASING_ON, MAX_VERTEX_MEMORY, MAX_ELEMENT_MEMORY)

#define nk_platform_shutdown() nk_sdl_shutdown()

#define nk_platform_font_stash_begin nk_sdl_font_stash_begin

#define nk_platform_font_stash_end nk_sdl_font_stash_end

#ifdef NK_SDL_GLES2_IMPLEMENTATION
void nk_platform_preinit()
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