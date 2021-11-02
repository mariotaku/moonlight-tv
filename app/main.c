#include <stdio.h>

#include "app.h"

#define RES_IMPL

#include "res.h"

#undef RES_IMPL

#include "lvgl.h"
#include "lvgl/lv_disp_drv_app.h"
#include "lvgl/lv_sdl_drv_key_input.h"
#include "lvgl/lv_sdl_drv_pointer_input.h"
#include "lvgl/lv_sdl_img.h"

#include "backend/backend_root.h"
#include "stream/session.h"
#include "stream/platform.h"
#include "ui/root.h"
#include "util/logging.h"

#include <ui/launcher/launcher.controller.h>
#include <lvgl/theme/lv_theme_moonlight.h>
#include <lvgl/lv_sdl_drv_wheel_input.h>
#include <SDL_image.h>

#if TARGET_WEBOS

#include "debughelper.h"

#define APP_FULLSCREEN_FLAG SDL_WINDOW_FULLSCREEN
#else
#define APP_FULLSCREEN_FLAG SDL_WINDOW_FULLSCREEN_DESKTOP
#endif

FILE *app_logfile = NULL;
SDL_Window *app_window = NULL;

static bool running = true;
static SDL_mutex *app_gs_client_mutex = NULL;

lv_controller_manager_t *app_uimanager;

static lv_indev_t *app_indev_key, *app_indev_wheel;

static void applog_logoutput(void *, int category, SDL_LogPriority priority, const char *message);

SDL_Window *app_create_window();

bool should_redir_output();

int main(int argc, char *argv[]) {
    app_loginit();
    SDL_LogSetOutputFunction(applog_logoutput, NULL);
#if TARGET_WEBOS
    bool should_redir = should_redir_output();
    if (should_redir) {
        app_logfile = fopen("/tmp/" APPID ".log", "a+");
    }
    if (!app_logfile)
        app_logfile = stdout;
    setvbuf(app_logfile, NULL, _IONBF, 0);
    if (should_redir)
        REDIR_STDOUT(APPID);
#endif
    applog_i("APP", "Start Moonlight. Version %s", APP_VERSION);
    app_gs_client_mutex = SDL_CreateMutex();

    int ret = app_init(argc, argv);
    if (ret != 0) {
        return ret;
    }
    module_host_context.logvprintf = &app_logvprintf;

    module_init(argc, argv);

    backend_init();

    // DO not init video subsystem before NDL/LGNC initialization
    app_init_video();
    app_window = app_create_window();

    module_post_init(argc, argv);

    lv_init();
    lv_disp_t *disp = lv_app_display_init(app_window);
    lv_theme_t *parent_theme = lv_disp_get_theme(disp);
    lv_theme_t theme_app = *parent_theme;
    lv_theme_set_parent(&theme_app, parent_theme);
    lv_theme_moonlight_init(&theme_app);
    lv_disp_set_theme(disp, &theme_app);
    streaming_display_size(disp->driver->hor_res, disp->driver->ver_res);

    lv_img_decoder_t *img_decoder = lv_img_decoder_create();
    lv_sdl_img_decoder_init(img_decoder);

    lv_group_t *group = lv_group_create();
    lv_group_set_editing(group, 0);
    lv_group_set_default(group);
    lv_indev_t *indev_key = lv_sdl_init_key_input();
    lv_indev_t *indev_wheel = lv_sdl_init_wheel();
    lv_indev_t *indev_pointer = lv_sdl_init_pointer();
    app_indev_key = indev_key;
    app_indev_wheel = indev_wheel;

    lv_obj_t *scr = lv_scr_act();
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(scr, 0, 0);
    app_uimanager = lv_controller_manager_create(scr, NULL);
    lv_controller_manager_push(app_uimanager, &launcher_controller_class, NULL);

    while (running) {
        app_process_events();
        lv_task_handler();
        SDL_Delay(1);
    }

    settings_save(app_configuration);

    lv_controller_manager_del(app_uimanager);

    lv_sdl_deinit_pointer(indev_pointer);
    lv_sdl_deinit_wheel(indev_wheel);
    lv_sdl_deinit_key_input(indev_key);
    lv_app_display_deinit(disp);
    lv_img_decoder_delete(img_decoder);
    lv_deinit();

    SDL_DestroyWindow(app_window);

    backend_destroy();
    decoder_finalize(decoder_current);
    audio_finalize(decoder_current);
    settings_free(app_configuration);

    SDL_DestroyMutex(app_gs_client_mutex);
    SDL_Quit();

    applog_i("APP", "Quitted gracefully :)");
    return 0;
}

bool should_redir_output() {
    const char *noredir = getenv("MOONLIGHT_OUTPUT_NOREDIR");
    return noredir == NULL || SDL_strncmp(noredir, "1", 1) != 0;
}

SDL_Window *app_create_window() {
    Uint32 window_flags = SDL_WINDOW_RESIZABLE;
    int window_width = 1920, window_height = 1080;
    if (app_configuration->fullscreen) {
        window_flags |= APP_FULLSCREEN_FLAG;
        SDL_DisplayMode mode;
        SDL_GetDisplayMode(0, 0, &mode);
        if (mode.w > 0 && mode.h > 0) {
            window_width = mode.w;
            window_height = mode.h;
        }
    }
    SDL_Window *win = SDL_CreateWindow("Moonlight", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                       window_width, window_height, window_flags);
    SDL_assert(win);
#if TARGET_DESKTOP || TARGET_RASPI
    SDL_Surface *winicon = IMG_Load_RW(SDL_RWFromConstMem(res_logo_96_data, (int) res_logo_96_size), SDL_TRUE);
    SDL_SetWindowIcon(win, winicon);
    SDL_FreeSurface(winicon);
#endif
    if (window_flags & SDL_WINDOW_RESIZABLE) {
        SDL_SetWindowMinimumSize(win, 960, 540);
    }
    int w = 0, h = 0;
    SDL_GetWindowSize(win, &w, &h);
    SDL_assert(w > 0 && h > 0);
    ui_display_size(w, h);
    return win;
}

void app_request_exit() {
    running = false;
}

GS_CLIENT app_gs_client_new() {
    SDL_LockMutex(app_gs_client_mutex);
    SDL_assert(app_configuration);
    GS_CLIENT client = gs_new(app_configuration->key_dir, app_configuration->debug_level);
    SDL_assert(client);
    SDL_UnlockMutex(app_gs_client_mutex);
    return client;
}

static void app_input_populate_group();

static lv_group_t *app_group = NULL, *app_modal_group = NULL;

void app_input_set_group(lv_group_t *group) {
    app_group = group;
    app_input_populate_group();
}

lv_group_t *app_input_get_group() {
    return app_group;
}

lv_group_t *app_input_get_modal_group() {
    return app_modal_group;
}

void app_input_set_modal_group(lv_group_t *group) {
    app_modal_group = group;
    app_input_populate_group();
}

void app_start_text_input(int x, int y, int w, int h) {
    if (w > 0 && h > 0) {
        struct SDL_Rect rect = {x, y, w, h};
        SDL_SetTextInputRect(&rect);
    } else {
        SDL_SetTextInputRect(NULL);
    }
    lv_sdl_key_input_release_key(app_indev_key);
    if (SDL_IsTextInputActive()) return;
    SDL_StartTextInput();
}

void applog_logoutput(void *userdata, int category, SDL_LogPriority priority, const char *message) {
    static const char *priority_name[SDL_NUM_LOG_PRIORITIES] = {"VERBOSE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"};
    static const char *category_name[SDL_LOG_CATEGORY_TEST + 1] = {
            "SDL.APPLICATION",
            "SDL.ERROR",
            "SDL.ASSERT",
            "SDL.SYSTEM",
            "SDL.AUDIO",
            "SDL.VIDEO",
            "SDL.RENDER",
            "SDL.INPUT",
            "SDL.TEST",
    };
#ifndef DEBUG
    if (priority <= SDL_LOG_PRIORITY_INFO)
        return;
#endif
    applog(priority_name[priority], category > SDL_LOG_CATEGORY_TEST ? "SDL" : category_name[category], message);
}

void app_set_fullscreen(bool fullscreen) {
    SDL_SetWindowFullscreen(app_window, fullscreen ? APP_FULLSCREEN_FLAG : 0);
}

static void app_input_populate_group() {
    lv_group_t *group = app_modal_group;
    if (!group) {
        group = app_group;
    }
    if (!group) {
        group = lv_group_get_default();
    }
    lv_indev_set_group(app_indev_key, group);
    lv_indev_set_group(app_indev_wheel, group);
}
