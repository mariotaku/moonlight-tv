#include <stdio.h>
#include <assert.h>
#include <pthread.h>

#include "app.h"
#define RES_IMPL
#include "res.h"
#undef RES_IMPL

#include "ui/config.h"

#include "lvgl.h"
#include "lvgl/lv_sdl_drv_display.h"
#include "lvgl/lv_sdl_drv_key_input.h"
#include "lvgl/lv_sdl_drv_pointer_input.h"

#include "debughelper.h"
#include "backend/backend_root.h"
#include "stream/session.h"
#include "stream/platform.h"
#include "ui/manager.h"
#include "ui/root.h"
#include "ui/fonts.h"
#include "util/bus.h"
#include "util/logging.h"

#include <fontconfig/fontconfig.h>

FILE *app_logfile = NULL;

static bool running = true;
static GS_CLIENT app_gs_client = NULL;
static pthread_mutex_t app_gs_client_mutex = PTHREAD_MUTEX_INITIALIZER;
static void app_gs_client_destroy();
static bool app_load_font(struct nk_context *ctx, struct nk_font_atlas *atlas);

int main(int argc, char *argv[])
{
    app_loginit();
#if TARGET_WEBOS || TARGET_LGNC
    app_logfile = fopen("/tmp/" APPID ".log", "a+");
    if (!app_logfile)
        app_logfile = stdout;
    setvbuf(app_logfile, NULL, _IONBF, 0);
    if (getenv("MOONLIGHT_OUTPUT_NOREDIR") == NULL)
        REDIR_STDOUT(APPID);
    applog_d("APP", "Start Moonlight. Version %s", APP_VERSION);
#endif
    bus_init();

    int ret = app_init(argc, argv);
    if (ret != 0)
    {
        return ret;
    }
    module_host_context.logvprintf = &app_logvprintf;

    // LGNC requires init before window created, don't put this after app_window_create!
    decoder_init(app_configuration->decoder, argc, argv);
    audio_init(app_configuration->audio_backend, argc, argv);

    applog_i("APP", "Decoder module: %s (%s requested)", decoder_definitions[decoder_current].name, app_configuration->decoder);
    if (audio_current == AUDIO_DECODER)
    {
        applog_i("APP", "Audio module: decoder implementation (%s requested)\n", app_configuration->audio_backend);
    }
    else if (audio_current >= 0)
    {
        applog_i("APP", "Audio module: %s (%s requested)", audio_definitions[audio_current].name, app_configuration->audio_backend);
    }

    backend_init();

    lv_init();
    lv_disp_t *disp = lv_sdl_init_display("Moonlight", 1920, 1080);
    streaming_display_size(disp->driver->hor_res, disp->driver->ver_res);

    lv_group_t *group = lv_group_create();
    lv_group_set_editing(group, 0);
    lv_group_set_default(group);
    lv_sdl_init_key_input();
    lv_sdl_init_pointer_input();

    ui_init();

    while (running)
    {
        app_process_events();
        lv_task_handler();
    }

    settings_save(app_configuration);

    lv_sdl_deinit_pointer_input();
    lv_sdl_deinit_key_input();
    lv_sdl_deinit_display();
    lv_deinit();

    backend_destroy();
    app_gs_client_destroy();
    app_destroy();
    bus_destroy();

    applog_d("APP", "Quitted gracefully :)");
}

void app_request_exit()
{
    running = false;
}

GS_CLIENT app_gs_client_obtain()
{
    pthread_mutex_lock(&app_gs_client_mutex);
    assert(app_configuration);
    if (!app_gs_client)
        app_gs_client = gs_new(app_configuration->key_dir, app_configuration->debug_level);
    assert(app_gs_client);
    pthread_mutex_unlock(&app_gs_client_mutex);
    return app_gs_client;
}

void app_gs_client_destroy()
{
    if (app_gs_client)
    {
        gs_destroy(app_gs_client);
        app_gs_client = NULL;
    }
    // Further calls to obtain gs client will be locked
    pthread_mutex_lock(&app_gs_client_mutex);
}

bool app_gs_client_ready()
{
    return app_gs_client != NULL;
}

bool app_load_font(struct nk_context *ctx, struct nk_font_atlas *atlas)
{
    FcConfig *config = FcInitLoadConfigAndFonts(); //Most convenient of all the alternatives
    if (!config)
        return false;

    //does not necessarily has to be a specific name.  You could put anything here and Fontconfig WILL find a font for you
    FcPattern *pat = FcNameParse((const FcChar8 *)FONT_FAMILY);
    if (!pat)
        goto deconfig;

    FcConfigSubstitute(config, pat, FcMatchPattern); //NECESSARY; it increases the scope of possible fonts
    FcDefaultSubstitute(pat);                        //NECESSARY; it increases the scope of possible fonts

    struct nk_font *font_ui = NULL;
    char *fontFile = NULL;
    FcResult result;

    FcPattern *font = FcFontMatch(config, pat, &result);
    if (!font)
        goto depat;
    //The pointer stored in 'file' is tied to 'font'; therefore, when 'font' is freed, this pointer is freed automatically.
    //If you want to return the filename of the selected font, pass a buffer and copy the file name into that buffer
    FcChar8 *file = NULL;

    if (FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch)
    {
        font_ui = nk_font_atlas_add_from_file_s(atlas, (char *)file, FONT_SIZE_DEFAULT, NULL);
        fonts_init(atlas, (char *)file);
        nk_style_set_font(ctx, &font_ui->handle);
    }

    FcPatternDestroy(font); //needs to be called for every pattern created; in this case, 'fontFile' / 'file' is also freed
depat:
    FcPatternDestroy(pat); //needs to be called for every pattern created
deconfig:
    FcConfigDestroy(config); //needs to be called for every config created
    return font_ui != NULL;
}