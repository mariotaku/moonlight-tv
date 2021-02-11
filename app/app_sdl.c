#include <stdbool.h>
#include <SDL.h>

#include "app.h"
#include "res.h"

#include "ui/config.h"

#define NK_IMPLEMENTATION
#include "nuklear/config.h"
#include "nuklear.h"
#include "nuklear/ext_functions.h"
#include "nuklear/ext_text.h"
#include "nuklear/ext_dialog.h"
#include "nuklear/ext_image.h"
#include "nuklear/ext_sprites.h"
#include "nuklear/ext_styling.h"
#include "nuklear/ext_imgview.h"
#include "nuklear/ext_smooth_list_view.h"
#include "nuklear/platform.h"

#if TARGET_DESKTOP || TARGET_RASPI
#include <SDL_image.h>
#endif

#include "backend/backend_root.h"
#include "stream/session.h"
#include "stream/input/absinput.h"
#include "stream/input/sdlinput.h"
#include "platform/sdl/events.h"
#include "platform/sdl/navkey_sdl.h"
#include "ui/root.h"
#include "util/user_event.h"

#if OS_WEBOS
#include "platform/webos/app_init.h"
#include "platform/webos/SDL_webOS.h"
#endif
#if TARGET_RASPI
#include "platform/raspi/app_init.h"
#endif

#define MAX_VERTEX_MEMORY 512 * 1024
#define MAX_ELEMENT_MEMORY 128 * 1024

SDL_mutex *mutex;
int sdlCurrentFrame, sdlNextFrame;

/* Platform */
SDL_Window *win;
SDL_GLContext gl;
static char wintitle[32];

static bool window_focus_gained;

static void fps_cap(int diff);

int app_init(int argc, char *argv[])
{
    app_configuration = settings_load();
#if OS_WEBOS
    return app_webos_init(argc, argv);
#else

    return 0;
#endif
}

APP_WINDOW_CONTEXT app_window_create()
{
    nk_platform_preinit();
    win = SDL_CreateWindow("Moonlight", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT,
                           SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);
#if TARGET_DESKTOP || TARGET_RASPI
    SDL_Surface *winicon = IMG_Load_RW(SDL_RWFromConstMem(res_window_icon_32_data, res_window_icon_32_size), SDL_TRUE);
    SDL_SetWindowIcon(win, winicon);
    SDL_FreeSurface(winicon);
    SDL_SetWindowMinimumSize(win, 640, 400);
#endif
#if OS_WEBOS
    app_webos_window_setup(win);
    {
        int refresh_rate, panel_width, panel_height;
        SDL_webOSGetRefreshRate(&refresh_rate);
        SDL_webOSGetPanelResolution(&panel_width, &panel_height);
        printf("webOS TV: refresh: %d Hz, panel: %d * %d\n", refresh_rate, panel_width, panel_height);
    }
#endif
#if TARGET_RASPI
    app_rpi_window_setup(win);
#endif
    SDL_Log("Video Driver: %s\n", SDL_GetCurrentVideoDriver());
    sdlCurrentFrame = sdlNextFrame = 0;
    mutex = SDL_CreateMutex();

    gl = SDL_GL_CreateContext(win);

    /* OpenGL setup */
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    return win;
}

void app_destroy()
{
    free(app_configuration);
#ifdef OS_WEBOS
    app_webos_destroy();
#endif
    SDL_GL_DeleteContext(gl);
    SDL_DestroyWindow(win);
    SDL_Quit();
}

void inputmgr_sdl_handle_event(SDL_Event ev);

static void app_process_events(struct nk_context *ctx)
{
    /* Input */
    SDL_Event evt;
    nk_input_begin(ctx);
    while (SDL_PollEvent(&evt))
    {
        bool block_steam_inputevent = false;
        if (evt.type == SDL_APP_WILLENTERBACKGROUND)
        {
            // Interrupt streaming because app will go to background
            streaming_interrupt(false);
        }
#if TARGET_DESKTOP || TARGET_RASPI
        else if (evt.type == SDL_WINDOWEVENT)
        {
            if (evt.window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
            {
                window_focus_gained = true;
            }
            else if (evt.window.event == SDL_WINDOWEVENT_FOCUS_LOST)
            {
                window_focus_gained = false;
            }
            else if (evt.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
            {
                ui_display_size(ctx, evt.window.data1, evt.window.data2);
            }
        }
#endif
        else if (SDL_IS_INPUT_EVENT(evt))
        {
            inputmgr_sdl_handle_event(evt);
            block_steam_inputevent |= ui_should_block_input();

            // Those are input events
            NAVKEY navkey = navkey_from_sdl(evt);
            if (navkey)
            {
                Uint32 timestamp = 0;
                bool is_key = false, is_gamepad = false;
                if (evt.type == SDL_KEYDOWN || evt.type == SDL_KEYUP)
                {
                    timestamp = evt.key.timestamp;
                    is_key = true;
                }
                else if (evt.type == SDL_CONTROLLERBUTTONDOWN || evt.type == SDL_CONTROLLERBUTTONUP)
                {
                    timestamp = evt.cbutton.timestamp;
                    is_gamepad = true;
                }
                if (is_key)
                {
                    ui_set_input_mode(UI_INPUT_MODE_KEY);
                }
                else if (is_gamepad)
                {
                    ui_set_input_mode(UI_INPUT_MODE_GAMEPAD);
                }
                bool down = evt.type == SDL_KEYDOWN || evt.type == SDL_CONTROLLERBUTTONDOWN;
#if OS_WEBOS
                if (down && (navkey & NAVKEY_DPAD))
                {
                    // Hide the cursor
                    SDL_webOSCursorVisibility(SDL_FALSE);
                }
#endif
                ui_dispatch_navkey(ctx, navkey, down, timestamp);
            }
            else if (evt.type == SDL_MOUSEMOTION)
            {
                ui_set_input_mode(UI_INPUT_MODE_POINTER);
            }
        }
        else if (evt.type == SDL_USEREVENT)
        {
            bool handled = backend_dispatch_userevent(evt.user.code, evt.user.data1, evt.user.data2);
            handled = handled || ui_dispatch_userevent(ctx, evt.user.code, evt.user.data1, evt.user.data2);
            if (!handled)
            {
                fprintf(stderr, "Nobody handles event %d\n", evt.user.code);
            }
        }
        else if (evt.type == SDL_QUIT)
        {
            app_request_exit();
        }
        if (!block_steam_inputevent)
        {
            absinput_dispatch_event(evt);
        }
        nk_sdl_handle_event(&evt);
    }
    nk_input_end(ctx);
}

void app_main_loop(void *data)
{
    static Uint32 fps_ticks = 0, framecount = 0;
    Uint32 start_ticks = SDL_GetTicks();
    struct nk_context *ctx = (struct nk_context *)data;

    app_process_events(ctx);

    bool cont = ui_root(ctx);

    /* Draw */
    {
        ui_render_background();
        nk_platform_render();
        SDL_GL_SwapWindow(win);
    }
#if OS_LINUX
    fps_cap(start_ticks);
#elif OS_DARWIN
    if (!window_focus_gained)
    {
        fps_cap(start_ticks);
    }
#endif
    if (!cont)
    {
        app_request_exit();
    }
    Uint32 end_ticks = SDL_GetTicks();
    Sint32 deltams = end_ticks - start_ticks;
    if (deltams < 0)
    {
        deltams = 0;
    }
    ctx->delta_time_seconds = deltams / 1000.0f;
    if ((end_ticks - fps_ticks) >= 1000)
    {
        sprintf(wintitle, "Moonlight | %d FPS", (int)(framecount * 1000.0 / (end_ticks - fps_ticks)));
        SDL_SetWindowTitle(win, wintitle);
        fps_ticks = end_ticks;
        framecount = 0;
    }
    else
    {
        framecount++;
    }
}

void app_start_text_input(int x, int y, int w, int h)
{
    if (w > 0 && h > 0)
    {
        struct SDL_Rect rect = {x, y, w, h};
        SDL_SetTextInputRect(&rect);
    }
    SDL_StartTextInput();
}

void app_stop_text_input()
{
    SDL_StopTextInput();
}

void fps_cap(int start)
{
    int tickdiff = SDL_GetTicks() - start;
    if (tickdiff > 0 && tickdiff < 16)
    {
        SDL_Delay(16 - tickdiff);
    }
}