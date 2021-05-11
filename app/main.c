#include <stdio.h>
#include <assert.h>
#include <pthread.h>

#include "app.h"
#define RES_IMPL
#include "res.h"
#undef RES_IMPL

#include "ui/config.h"

#include "nuklear/config.h"
#include "nuklear.h"
#include "nuklear/ext_functions.h"
#include "nuklear/ext_image.h"
#include "nuklear/ext_sprites.h"
#include "nuklear/ext_styling.h"
#include "nuklear/platform.h"

#include "debughelper.h"
#include "backend/backend_root.h"
#include "stream/session.h"
#include "stream/platform.h"
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

int main(int argc, char *argv[])
{
    app_loginit();
#if TARGET_WEBOS || TARGET_LGNC
    app_logfile = fopen("/tmp/" APPID ".log", "a+");
    setvbuf(app_logfile, NULL, _IONBF, 0);
    if (getenv("MOONLIGHT_OUTPUT_NOREDIR") == NULL)
        REDIR_STDOUT(APPID);
    applog_d("APP", "main() init");
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

    /* GUI */
    struct nk_context *ctx;
    APP_WINDOW_CONTEXT win = app_window_create();
    if (!win)
    {
        applog_e("APP", "Failed to create window");
        return 1;
    }

    applog_i("APP", "Decoder module: %s", decoder_definitions[decoder_current].name);
    if (audio_current == AUDIO_DECODER)
    {
        applog_i("APP", "Audio module: decoder implementation\n");
    }
    else if (audio_current >= 0)
    {
        applog_i("APP", "Audio module: %s", audio_definitions[audio_current].name);
    }

    backend_init();

    ctx = nk_platform_init(win);
    ui_display_size(app_window_width, app_window_height);
    streaming_display_size(app_window_width, app_window_height);
    {
        FcInit();
        FcConfig *config = FcInitLoadConfigAndFonts(); //Most convenient of all the alternatives

        //does not necessarily has to be a specific name.  You could put anything here and Fontconfig WILL find a font for you
        FcPattern *pat = FcNameParse((const FcChar8 *)"Sans Serif");

        FcConfigSubstitute(config, pat, FcMatchPattern); //NECESSARY; it increases the scope of possible fonts
        FcDefaultSubstitute(pat);                        //NECESSARY; it increases the scope of possible fonts

        struct nk_font_atlas *atlas;
        nk_platform_font_stash_begin(&atlas);

        struct nk_font *font_ui_20 = NULL;
        char *fontFile = NULL;
        FcResult result;

        FcPattern *font = FcFontMatch(config, pat, &result);
        if (font)
        {
            //The pointer stored in 'file' is tied to 'font'; therefore, when 'font' is freed, this pointer is freed automatically.
            //If you want to return the filename of the selected font, pass a buffer and copy the file name into that buffer
            FcChar8 *file = NULL;

            if (FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch)
            {
                fontFile = (char *)file;
                font_ui_20 = nk_font_atlas_add_from_file_s(atlas, fontFile, 20, NULL);
            }
        }
        if (!font_ui_20)
        {
            font_ui_20 = nk_font_atlas_add_default(atlas, 20 * NK_UI_SCALE, NULL);
        }
        fonts_init(atlas, fontFile);
        nk_platform_font_stash_end();
        nk_style_set_font(ctx, &font_ui_20->handle);
#if DEBUG && TARGET_DESKTOP
        nk_style_load_all_cursors(ctx, atlas->cursors);
#endif
        FcPatternDestroy(font);  //needs to be called for every pattern created; in this case, 'fontFile' / 'file' is also freed
        FcPatternDestroy(pat);   //needs to be called for every pattern created
        FcConfigDestroy(config); //needs to be called for every config created
        FcFini();
    }
    nk_ext_sprites_init();
    nk_ext_apply_style(ctx);

    ui_root_init(ctx);

    while (running)
    {
        app_main_loop((void *)ctx);
    }

    settings_save(app_configuration);

    ui_root_destroy();

    nk_ext_sprites_destroy();

    nk_platform_shutdown();
    backend_destroy();
    app_gs_client_destroy();
    app_destroy();
    bus_destroy();

    applog_d("APP", "Quitted gracefully :)");
    return 0;
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
        pthread_mutex_lock(&app_gs_client_mutex);
        // Further calls to obtain gs client will be locked
    }
}

bool app_gs_client_ready()
{
    return app_gs_client != NULL;
}