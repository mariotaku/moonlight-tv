#include <SDL_image.h>
#include "app.h"
#include "logging.h"
#include "logging_ext_sdl.h"

#include "res.h"

#include "lvgl.h"
#include "lvgl/lv_disp_drv_app.h"
#include "lvgl/lv_sdl_drv_input.h"
#include "lvgl/theme/lv_theme_moonlight.h"
#include "lv_sdl_img.h"

#include "backend/backend_root.h"
#include "stream/session.h"
#include "stream/platform.h"
#include "ui/root.h"
#include "ui/launcher/launcher.controller.h"

#include "util/font.h"
#include "util/i18n.h"
#include "util/bus.h"

#include "ss4s.h"
#include "input/app_input.h"
#include "errors.h"
#include "ui/fatal_error.h"

static bool running = true;
static SDL_mutex *app_gs_client_mutex = NULL;

lv_fragment_manager_t *app_uimanager;



static void log_libs_version();

static SDL_AssertState app_assertion_handler_abort(const SDL_AssertData *data, void *userdata);

static SDL_AssertState app_assertion_handler_ui(const SDL_AssertData *data, void *userdata);

app_t *global = NULL;

int main(int argc, char *argv[]) {
    commons_logging_init("moonlight");
    SDL_LogSetOutputFunction(commons_sdl_log, NULL);
    SDL_SetAssertionHandler(app_assertion_handler_abort, NULL);
    commons_log_info("APP", "Start Moonlight. Version %s", APP_VERSION);
    log_libs_version();
    app_gs_client_mutex = SDL_CreateMutex();

    app_t app_ = {
            .main_thread_id = SDL_ThreadID()
    };
    int ret = app_init(&app_, argc, argv);
    if (ret != 0) {
        return ret;
    }
    app_init_locale();
    backend_init(&app_.backend);

    // DO not init video subsystem before NDL/LGNC initialization
    app_init_video();
    commons_log_info("APP", "UI locale: %s (%s)", i18n_locale(), locstr("[Localized Language]"));

    app_ui_init(&app_.ui, &app_);

    global = &app_;

    SS4S_PostInit(argc, argv);

    app_handle_launch(argc, argv);

    if (strlen(app_configuration->default_host_uuid) > 0) {
        commons_log_info("APP", "Will launch with host %s and app %d", app_configuration->default_host_uuid,
                         app_configuration->default_app_id);
    }

    lv_disp_t *disp = lv_app_display_init(app_.ui.window);
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


    lv_group_t *group = lv_group_create();
    lv_group_set_editing(group, 0);
    lv_group_set_default(group);
    app_input_init(&app_.input, &app_);

    SDL_SetAssertionHandler(app_assertion_handler_ui, &app_);

    lv_obj_t *scr = lv_scr_act();
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(scr, 0, 0);
    app_uimanager = lv_fragment_manager_create(NULL);
    lv_fragment_t *fragment = lv_fragment_create(&launcher_controller_class, &app_);
    lv_fragment_manager_push(app_uimanager, fragment, &scr);


    while (app_is_running()) {
        app_process_events(&app_);
        lv_task_handler();
        SDL_Delay(1);
    }

    SDL_SetAssertionHandler(NULL, NULL);

    lv_fragment_manager_del(app_uimanager);

    app_input_deinit(&app_.input);
    lv_app_display_deinit(disp);
    app_font_deinit(fonts);

    app_ui_deinit(&app_.ui);
    app_uninit_video();

    backend_destroy(&app_.backend);

    bus_finalize();

    settings_save(app_configuration);
    settings_free(app_configuration);

    SDL_DestroyMutex(app_gs_client_mutex);

    app_deinit(&app_);

    _lv_draw_mask_cleanup();

    SDL_Quit();

    commons_log_info("APP", "Quitted gracefully :)");
    return 0;
}

void app_request_exit() {
    running = false;
}

bool app_is_running() {
    return running;
}

GS_CLIENT app_gs_client_new() {
    if (SDL_ThreadID() == global->main_thread_id) {
        commons_log_fatal("APP", "%s MUST BE called from worker thread!", __FUNCTION__);
        abort();
    }
    SDL_LockMutex(app_gs_client_mutex);
    SDL_assert_release(app_configuration != NULL);
    GS_CLIENT client = gs_new(app_configuration->key_dir, app_configuration->debug_level);
    if (client == NULL && gs_get_error(NULL) == GS_BAD_CONF) {
        if (gs_conf_init(app_configuration->key_dir) != GS_OK) {
            const char *message = NULL;
            gs_get_error(&message);
            app_fatal_error("Failed to generate client info",
                            "Please try reinstall the app.\n\nDetails: %s", message);
            app_halt();
        } else {
            client = gs_new(app_configuration->key_dir, app_configuration->debug_level);
        }
    }
    if (client == NULL) {
        const char *message = NULL;
        gs_get_error(&message);
        app_fatal_error("Fatal error", "Failed to create GS_CLIENT: %s", message);
        app_halt();
    }
    SDL_UnlockMutex(app_gs_client_mutex);
    return client;
}

static void log_libs_version() {
    SDL_version sdl_version;
    SDL_GetVersion(&sdl_version);
    commons_log_debug("APP", "SDL version: %d.%d.%d", sdl_version.major, sdl_version.minor, sdl_version.patch);
    const SDL_version *img_version = IMG_Linked_Version();
    commons_log_debug("APP", "SDL_image version: %d.%d.%d", img_version->major, img_version->minor, img_version->patch);
}

static SDL_AssertState app_assertion_handler_abort(const SDL_AssertData *data, void *userdata) {
    (void) userdata;
    commons_log_fatal("Assertion", "at %s (%s:%d): '%s'", data->function, data->filename, data->linenum,
                      data->condition);
    return SDL_ASSERTION_ABORT;
}

static SDL_AssertState app_assertion_handler_ui(const SDL_AssertData *data, void *userdata) {
    (void) userdata;
    app_fatal_error("Assertion failure", "at %s\n(%s:%d): '%s'", data->function, data->filename, data->linenum,
                    data->condition);
    return SDL_ASSERTION_ALWAYS_IGNORE;
}
