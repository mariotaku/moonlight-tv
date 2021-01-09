#pragma once
#include "nuklear_lgnc_gles2.h"

#define nk_platform_init(appctx) nk_lgnc_init()

#define nk_platform_shutdown() nk_lgnc_shutdown()

#define nk_platform_font_stash_begin nk_lgnc_font_stash_begin

#define nk_platform_font_stash_end nk_lgnc_font_stash_end

#ifdef NK_LGNC_GLES2_IMPLEMENTATION
static void nk_platform_gl_setup()
{

}
#endif