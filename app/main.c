/* nuklear - 1.40.8 - public domain */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <limits.h>
#include <time.h>

#define NK_IMPLEMENTATION
#include "nuklear/config.h"
#include "nuklear.h"
#include "demo/sdl_opengles2/nuklear_sdl_gles2.h"

#include <gst/gst.h>
#include <NDL_directmedia.h>

#include "main.h"
#include "debughelper.h"
#include "gst_demo.h"
#include "backend/computer_manager.h"
#include "ui/gui_root.h"

static const char APPID[] = WEBOS_APPID;

#define WINDOW_WIDTH 960
#define WINDOW_HEIGHT 540

#define MAX_VERTEX_MEMORY 512 * 1024
#define MAX_ELEMENT_MEMORY 128 * 1024

#define UNUSED(a) (void)a
#define LEN(a) (sizeof(a) / sizeof(a)[0])

/* Platform */
SDL_Window *win;
int running = nk_true;

static void
MainLoop(void *loopArg)
{
    struct nk_context *ctx = (struct nk_context *)loopArg;

    /* Input */
    SDL_Event evt;
    nk_input_begin(ctx);
    while (SDL_PollEvent(&evt))
    {
        if (evt.type == SDL_QUIT)
            running = nk_false;
        nk_sdl_handle_event(&evt);
    }
    nk_input_end(ctx);

    bool cont = gui_root(ctx);

    /* Draw */
    {
        gui_background();
        /* IMPORTANT: `nk_sdl_render` modifies some global OpenGL state
     * with blending, scissor, face culling, depth test and viewport and
     * defaults everything back into a default state.
     * Make sure to either a.) save and restore or b.) reset your own state after
     * rendering the UI. */
        nk_sdl_render(NK_ANTI_ALIASING_ON, MAX_VERTEX_MEMORY, MAX_ELEMENT_MEMORY);
        SDL_GL_SwapWindow(win);
    }
    if (!cont)
    {
        request_exit();
    }
}

int main(int argc, char *argv[])
{
    REDIR_STDOUT("moonlight");

    gst_init(&argc, &argv);

    if (NDL_DirectMediaInit(APPID, NULL))
    {
        SDL_Log("Unable to initialize NDL\n", NDL_DirectMediaGetError());
        return -1;
    }

    computer_manager_init();

    /* GUI */
    struct nk_context *ctx;
    SDL_GLContext glContext;
    /* SDL setup */
    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");
    /*SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_EVENTS); // - do NOT init SDL on GL ES 2 */
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    win = SDL_CreateWindow("Demo",
                           SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                           WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);
    glContext = SDL_GL_CreateContext(win);

    /* OpenGL setup */
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    ctx = nk_sdl_init(win);
    /* Load Fonts: if none of these are loaded a default font will be used  */
    /* Load Cursor: if you uncomment cursor loading please hide the cursor */
    {
        struct nk_font_atlas *atlas;
        nk_sdl_font_stash_begin(&atlas);
        /*struct nk_font *droid = nk_font_atlas_add_from_file(atlas, "../../../extra_font/DroidSans.ttf", 14, 0);*/
        /*struct nk_font *roboto = nk_font_atlas_add_from_file(atlas, "../../../extra_font/Roboto-Regular.ttf", 16, 0);*/
        /*struct nk_font *future = nk_font_atlas_add_from_file(atlas, "../../../extra_font/kenvector_future_thin.ttf", 13, 0);*/
        /*struct nk_font *clean = nk_font_atlas_add_from_file(atlas, "../../../extra_font/ProggyClean.ttf", 12, 0);*/
        /*struct nk_font *tiny = nk_font_atlas_add_from_file(atlas, "../../../extra_font/ProggyTiny.ttf", 10, 0);*/
        /*struct nk_font *cousine = nk_font_atlas_add_from_file(atlas, "../../../extra_font/Cousine-Regular.ttf", 13, 0);*/
        nk_sdl_font_stash_end();
        /*nk_style_load_all_cursors(ctx, atlas->cursors);*/
        /*nk_style_set_font(ctx, &roboto->handle)*/;
    }

    /* style.c */
    /*set_style(ctx, THEME_WHITE);*/
    /*set_style(ctx, THEME_RED);*/
    /*set_style(ctx, THEME_BLUE);*/
    /*set_style(ctx, THEME_DARK);*/
    gui_root_init(ctx);

    gst_demo_initialize();

    while (running)
        MainLoop((void *)ctx);

    nk_sdl_shutdown();

    computer_manager_destroy();

    gst_demo_finalize();
    
    NDL_DirectMediaQuit();
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}

void request_exit()
{
    running = nk_false;
}

int exit_requested()
{
    return !running;
}