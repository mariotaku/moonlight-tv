#include "app.h"

#include "res.h"

#include "lvgl.h"
#include "lvgl/lv_disp_drv_app.h"
#include "lvgl/lv_sdl_drv_input.h"
#include "lvgl/lv_sdl_img.h"
#include "lvgl/theme/lv_theme_moonlight.h"

#include "backend/backend_root.h"
#include "stream/session.h"
#include "stream/platform.h"
#include "ui/root.h"
#include "ui/launcher/launcher.controller.h"

#include "util/font.h"
#include "util/i18n.h"
#include "util/logging.h"
#include "util/bus.h"

#include "ss4s.h"
#include "input/app_input.h"

#include <SDL_image.h>

#if TARGET_WEBOS

#define APP_FULLSCREEN_FLAG SDL_WINDOW_FULLSCREEN
#else
#define APP_FULLSCREEN_FLAG SDL_WINDOW_FULLSCREEN_DESKTOP
#endif

static bool running = true;
static SDL_mutex *app_gs_client_mutex = NULL;

lv_fragment_manager_t *app_uimanager;


SDL_Window *app_create_window();

static void applog_logoutput(void *, int category, SDL_LogPriority priority, const char *message);

static void applog_lvlog(const char *msg);

static void log_libs_version();

static void applog_ss4s(SS4S_LogLevel level, const char *tag, const char *fmt, ...);

int main(int argc, char *argv[]) {
    app_loginit();
    SDL_LogSetOutputFunction(applog_logoutput, NULL);
    applog_i("APP", "Start Moonlight. Version %s", APP_VERSION);
    log_libs_version();
    app_gs_client_mutex = SDL_CreateMutex();

    app_t app_;
    int ret = app_init(&app_, argc, argv);
    if (ret != 0) {
        return ret;
    }
    app_init_locale();
    backend_init();

    // DO not init video subsystem before NDL/LGNC initialization
    app_init_video();
    applog_i("APP", "UI locale: %s (%s)", i18n_locale(), locstr("[Localized Language]"));

    // TODO: force set fullscreen if decoder doesn't support windowed mode
    app_configuration->fullscreen = true;

    app_.window = app_create_window();

    SS4S_PostInit(argc, argv);

    app_handle_launch(argc, argv);

    if (strlen(app_configuration->default_host_uuid) > 0) {
        applog_i("APP", "Will launch with host %s and app %d", app_configuration->default_host_uuid,
                 app_configuration->default_app_id);
    }

    lv_log_register_print_cb(applog_lvlog);
    lv_init();
    lv_disp_t *disp = lv_app_display_init(app_.window);
    lv_theme_t *parent_theme = lv_disp_get_theme(disp);
    lv_theme_t theme_app;
    lv_memset_00(&theme_app, sizeof(lv_theme_t));
    theme_app.color_primary = parent_theme->color_primary;
    theme_app.color_secondary = parent_theme->color_secondary;
    lv_theme_set_parent(&theme_app, parent_theme);
    lv_theme_moonlight_init(&theme_app, &app_);
    app_fonts_t *fonts = app_font_init(&theme_app);
    lv_disp_set_theme(disp, &theme_app);
    streaming_display_size(disp->driver->hor_res, disp->driver->ver_res);

    lv_img_decoder_t *img_decoder = lv_sdl_img_decoder_init(IMG_INIT_JPG | IMG_INIT_PNG);

    lv_group_t *group = lv_group_create();
    lv_group_set_editing(group, 0);
    lv_group_set_default(group);
    app_input_init(&app_.input, &app_);

    lv_obj_t *scr = lv_scr_act();
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(scr, 0, 0);
    app_uimanager = lv_fragment_manager_create(NULL);
    lv_fragment_t *fragment = lv_fragment_create(&launcher_controller_class, &app_);
    lv_fragment_manager_push(app_uimanager, fragment, &scr);

    while (app_is_running()) {
        app_process_events();
        lv_task_handler();
        SDL_Delay(1);
    }

    lv_fragment_manager_del(app_uimanager);

    app_input_deinit(&app_.input);
    lv_app_display_deinit(disp);
    lv_img_decoder_delete(img_decoder);
    app_font_deinit(fonts);

    if (!app_configuration->fullscreen) {
        SDL_GetWindowPosition(app_.window, &app_configuration->window_state.x, &app_configuration->window_state.y);
        SDL_GetWindowSize(app_.window, &app_configuration->window_state.w, &app_configuration->window_state.h);
    }

    SDL_DestroyWindow(app_.window);
    app_uninit_video();

    backend_destroy();

    bus_finalize();

    settings_save(app_configuration);
    settings_free(app_configuration);

    SDL_DestroyMutex(app_gs_client_mutex);

    app_deinit(&app_);

    SDL_Quit();

    applog_i("APP", "Quitted gracefully :)");
    return 0;
}

SDL_Window *app_create_window() {
    Uint32 win_flags = SDL_WINDOW_RESIZABLE;
    int win_x = SDL_WINDOWPOS_UNDEFINED, win_y = SDL_WINDOWPOS_UNDEFINED,
            win_width = 1920, win_height = 1080;
    if (app_configuration->fullscreen) {
        win_flags |= APP_FULLSCREEN_FLAG;
        SDL_DisplayMode mode;
        SDL_GetDisplayMode(0, 0, &mode);
        if (mode.w > 0 && mode.h > 0) {
            win_width = mode.w;
            win_height = mode.h;
        }
    } else {
        win_x = app_configuration->window_state.x;
        win_y = app_configuration->window_state.y;
        win_width = app_configuration->window_state.w;
        win_height = app_configuration->window_state.h;
    }
    SDL_Window *win = SDL_CreateWindow("Moonlight", win_x, win_y, win_width, win_height, win_flags);
    SDL_assert(win != NULL);

    SDL_Surface *winicon = IMG_Load_RW(SDL_RWFromConstMem(lv_sdl_img_data_logo_96.data.constptr,
                                                          (int) lv_sdl_img_data_logo_96.data_len), SDL_TRUE);
    SDL_SetWindowIcon(win, winicon);
    SDL_FreeSurface(winicon);
    SDL_SetWindowMinimumSize(win, 640, 480);
    int w = 0, h = 0;
    SDL_GetWindowSize(win, &w, &h);
    SDL_assert(w > 0 && h > 0);
    ui_display_size(w, h);
    return win;
}

void app_request_exit() {
    running = false;
}

bool app_is_running() {
    return running;
}

GS_CLIENT app_gs_client_new() {
    SDL_LockMutex(app_gs_client_mutex);
    SDL_assert(app_configuration);
    GS_CLIENT client = gs_new(app_configuration->key_dir, app_configuration->debug_level);
    if (client == NULL) {
        applog_f("APP", "Failed to create GameStream client: %s", gs_error);
    }
    SDL_assert(client);
    SDL_UnlockMutex(app_gs_client_mutex);
    return client;
}


void applog_logoutput(void *userdata, int category, SDL_LogPriority priority, const char *message) {
    static const applog_level_t priority_name[SDL_NUM_LOG_PRIORITIES] = {APPLOG_VERBOSE, APPLOG_DEBUG, APPLOG_INFO,
                                                                         APPLOG_WARN, APPLOG_ERROR, APPLOG_FATAL};
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
    applog(priority_name[priority - SDL_LOG_PRIORITY_VERBOSE],
           category > SDL_LOG_CATEGORY_TEST ? "SDL" : category_name[category], "%s", message);
}

static void applog_lvlog(const char *msg) {
    const char *start = strchr(msg, '\t') + 1;
    static const applog_level_t level_value[] = {
            APPLOG_VERBOSE, APPLOG_INFO, APPLOG_WARN, APPLOG_ERROR, APPLOG_DEBUG,
    };
    static const char *level_name[] = {
            "Trace", "Info", "Warn", "Error", "User",
    };
    for (int i = 0; i < sizeof(level_value) / sizeof(applog_level_t); i++) {
        if (strncmp(level_name[i], msg + 1, (start - msg - 3)) == 0) {
            applog(level_value[i], "LVGL", "%s", start);
        }
    }
}

void app_set_fullscreen(app_t *app, bool fullscreen) {
    SDL_SetWindowFullscreen(app->window, fullscreen ? APP_FULLSCREEN_FLAG : 0);
}


static void log_libs_version() {
    SDL_version sdl_version;
    SDL_GetVersion(&sdl_version);
    applog_d("APP", "SDL version: %d.%d.%d", sdl_version.major, sdl_version.minor, sdl_version.patch);
    const SDL_version *img_version = IMG_Linked_Version();
    applog_d("APP", "SDL_image version: %d.%d.%d", img_version->major, img_version->minor, img_version->patch);
}
