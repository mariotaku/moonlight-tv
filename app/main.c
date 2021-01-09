#include "nuklear/config.h"
#include "nuklear.h"
#include "nuklear/ext_widgets.h"
#include "nuklear/ext_functions.h"

#if defined(NK_SDL_GLES2)
#include "nuklear/platform_sdl_gles2.h"
#elif defined(NK_SDL_GL2)
#include "nuklear/platform_sdl_gl2.h"
#elif defined(NK_LGNC_GLES2)
#else
#error "No valid render backend specified"
#endif

#ifdef OS_WEBOS
#include "sdl/webos_keys.h"
#endif

#include "main.h"
#include "app.h"
#include "debughelper.h"
#include "backend/backend_root.h"
#include "stream/session.h"
#include "ui/config.h"
#include "ui/gui_root.h"

bool running = true;

int main(int argc, char *argv[])
{
#ifdef OS_WEBOS
    REDIR_STDOUT("moonlight");
#endif
    int ret = app_init();
    if (ret != 0)
    {
        return ret;
    }

    backend_init();

    /* GUI */
    struct nk_context *ctx;
    APP_WINDOW_CONTEXT appctx = app_window_create();
    streaming_display_size(WINDOW_WIDTH, WINDOW_HEIGHT);
    gui_display_size(WINDOW_WIDTH, WINDOW_HEIGHT);

    ctx = nk_platform_init(appctx);
    /* Load Fonts: if none of these are loaded a default font will be used  */
    /* Load Cursor: if you uncomment cursor loading please hide the cursor */
    {
        struct nk_font_atlas *atlas;
        nk_platform_font_stash_begin(&atlas);
        /*struct nk_font *droid = nk_font_atlas_add_from_file(atlas, "../../../extra_font/DroidSans.ttf", 14, 0);*/
        /*struct nk_font *roboto = nk_font_atlas_add_from_file(atlas, "../../../extra_font/Roboto-Regular.ttf", 16, 0);*/
        /*struct nk_font *future = nk_font_atlas_add_from_file(atlas, "../../../extra_font/kenvector_future_thin.ttf", 13, 0);*/
        /*struct nk_font *clean = nk_font_atlas_add_from_file(atlas, "../../../extra_font/ProggyClean.ttf", 12, 0);*/
        /*struct nk_font *tiny = nk_font_atlas_add_from_file(atlas, "../../../extra_font/ProggyTiny.ttf", 10, 0);*/
        /*struct nk_font *cousine = nk_font_atlas_add_from_file(atlas, "../../../extra_font/Cousine-Regular.ttf", 13, 0);*/
        nk_platform_font_stash_end();
        /*nk_style_load_all_cursors(ctx, atlas->cursors);*/
        /*nk_style_set_font(ctx, &roboto->handle)*/;
    }

    /* style.c */
    /*set_style(ctx, THEME_WHITE);*/
    /*set_style(ctx, THEME_RED);*/
    /*set_style(ctx, THEME_BLUE);*/
    /*set_style(ctx, THEME_DARK);*/
    gui_root_init(ctx);

    while (running)
    {
        app_main_loop((void *)ctx);
    }

    nk_platform_shutdown();

    backend_destroy();

    app_destroy();
    return 0;
}

void request_exit()
{
    running = false;
}