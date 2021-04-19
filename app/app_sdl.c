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
#include "nuklear/ext_text_multiline.h"
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
#include "stream/platform.h"
#include "platform/sdl/events.h"
#include "platform/sdl/navkey_sdl.h"
#include "ui/root.h"
#include "util/bus.h"
#include "util/user_event.h"

#if TARGET_WEBOS
#include "platform/webos/app_init.h"
#define FORCE_FULLSCREEN
#elif TARGET_RASPI
#define FORCE_FULLSCREEN
#endif

#define MAX_VERTEX_MEMORY 512 * 1024
#define MAX_ELEMENT_MEMORY 128 * 1024

/* Platform */
SDL_Window *win;
static SDL_GLContext gl;
int app_window_width, app_window_height;
bool app_has_redraw = false, app_force_redraw = false, app_should_redraw_background = false;

PCONFIGURATION app_configuration = NULL;

static char wintitle[32];

static bool window_focus_gained;

static void fps_cap(int diff);

int app_init(int argc, char *argv[])
{
#if TARGET_WEBOS
    app_webos_init(argc, argv);
#endif
    app_configuration = settings_load();
    return 0;
}

APP_WINDOW_CONTEXT app_window_create()
{
    nk_platform_preinit();
    Uint32 window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI;
#ifdef FORCE_FULLSCREEN
    SDL_DisplayMode dm;
    SDL_GetCurrentDisplayMode(0, &dm);
    app_window_width = dm.w;
    app_window_height = dm.h;
    window_flags |= SDL_WINDOW_FULLSCREEN;
#else
    app_window_width = WINDOW_WIDTH;
    app_window_height = WINDOW_HEIGHT;
    window_flags |= SDL_WINDOW_RESIZABLE;
#endif
    win = SDL_CreateWindow("Moonlight", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                           app_window_width, app_window_height, window_flags);
#if TARGET_DESKTOP || TARGET_RASPI
    SDL_Surface *winicon = IMG_Load_RW(SDL_RWFromConstMem(res_window_icon_32_data, res_window_icon_32_size), SDL_TRUE);
    SDL_SetWindowIcon(win, winicon);
    SDL_FreeSurface(winicon);
    SDL_SetWindowMinimumSize(win, 640, 400);
#endif
    SDL_Log("Video Driver: %s\n", SDL_GetCurrentVideoDriver());

    gl = SDL_GL_CreateContext(win);

    /* OpenGL setup */
    glViewport(0, 0, app_window_width, app_window_height);
    return win;
}

void app_destroy()
{
    free(app_configuration);
    decoder_finalize(decoder_current);
#ifdef TARGET_WEBOS
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
    if (app_has_redraw)
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
                ui_display_size(evt.window.data1, evt.window.data2);
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
                NAVKEY_STATE state;
                if (is_key)
                {
                    state = evt.key.state == SDL_PRESSED;
                    if (evt.key.repeat)
                        state |= NAVKEY_STATE_REPEAT;
                    else
                        ui_set_input_mode(UI_INPUT_MODE_KEY);
                }
                else if (is_gamepad)
                {
                    state = evt.cbutton.state == SDL_PRESSED;
                    ui_set_input_mode(UI_INPUT_MODE_GAMEPAD);
                }

#if TARGET_WEBOS
                if (state == NAVKEY_STATE_DOWN && (navkey & NAVKEY_DPAD))
                {
                    // Hide the cursor
                    SDL_webOSCursorVisibility(SDL_FALSE);
                }
#endif
                ui_dispatch_navkey(ctx, navkey, state, timestamp);
            }
            else if (evt.type == SDL_MOUSEMOTION)
            {
                ui_set_input_mode(UI_INPUT_MODE_POINTER);
            }
#if TARGET_WEBOS
            else if (evt.type == SDL_KEYDOWN && evt.key.keysym.scancode == SDL_WEBOS_SCANCODE_EXIT)
            {
                if (!streaming_running)
                {
                    app_request_exit();
                }
            }
#endif
        }
        else if (evt.type == SDL_USEREVENT)
        {
            if (evt.user.code == BUS_INT_EVENT_ACTION)
            {
                bus_actionfunc actionfn = evt.user.data1;
                actionfn(evt.user.data2);
            }
            else
            {
                bool handled = backend_dispatch_userevent(evt.user.code, evt.user.data1, evt.user.data2);
                handled = handled || ui_dispatch_userevent(ctx, evt.user.code, evt.user.data1, evt.user.data2);
                if (!handled)
                {
                    fprintf(stderr, "Nobody handles event %d\n", evt.user.code);
                }
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
        if (app_has_redraw)
            nk_sdl_handle_event(&evt);
    }
    if (app_has_redraw)
        nk_input_end(ctx);
}

void app_main_loop(void *data)
{
    static Uint32 fps_ticks = 0, framecount = 0;
    Uint32 start_ticks = SDL_GetTicks();
    struct nk_context *ctx = (struct nk_context *)data;

    app_process_events(ctx);

    app_has_redraw = ui_root(ctx) || app_force_redraw;

    /* Draw */

    if (app_has_redraw)
    {
        ui_render_background();
        nk_platform_render();
        SDL_GL_SwapWindow(win);
        app_should_redraw_background = true;
#if OS_LINUX
        fps_cap(start_ticks);
#elif OS_DARWIN
        if (!window_focus_gained)
            fps_cap(start_ticks);
#endif
    }
    else if (app_should_redraw_background)
    {
        ui_render_background();
        SDL_GL_SwapWindow(win);
        app_should_redraw_background = false;
    }
    else
    {
        // Just delay for 1ms so we can get ~1000 loops per second for input events
        SDL_Delay(1);
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

bool app_render_queue_submit(void *data)
{
    SDL_Event event;
    event.type = SDL_USEREVENT;
    event.user.code = USER_SDL_FRAME;
    event.user.data1 = data;
    SDL_PushEvent(&event);
    return true;
}

void fps_cap(int start)
{
    int tickdiff = SDL_GetTicks() - start;
    if (tickdiff > 0 && tickdiff < 16)
    {
        SDL_Delay(16 - tickdiff);
    }
}