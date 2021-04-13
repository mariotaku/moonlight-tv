#include "window.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#if OS_WEBOS
#include "platform/webos/os_info.h"
#if HAVE_SDL
#include <SDL_webOS.h>
#endif
#endif

#include "stream/platform.h"

#if OS_WEBOS
static char webos_release[32];
static struct
{
    int w, h;
    int rate;
} webos_panel_info;
#endif
static PLATFORM _audio_platform = NONE;

static void load_webos_info();

static bool _render(struct nk_context *ctx, bool *showing_combo)
{
    static struct nk_rect item_bounds = {0, 0, 0, 0};
    int item_index = 0;

    nk_layout_row_template_begin_s(ctx, 25);
    nk_layout_row_template_push_static_s(ctx, 120);
    nk_layout_row_template_push_variable_s(ctx, 1);
    nk_layout_row_template_end(ctx);
    nk_label(ctx, "Version", NK_TEXT_LEFT);
    nk_label(ctx, APP_VERSION, NK_TEXT_RIGHT);
    nk_label(ctx, "Video Decoder", NK_TEXT_LEFT);
    nk_label(ctx, platform_definitions[platform_current].name, NK_TEXT_RIGHT);
    nk_label(ctx, "Audio Decoder", NK_TEXT_LEFT);
    nk_label(ctx, platform_definitions[_audio_platform].name, NK_TEXT_RIGHT);

#if OS_WEBOS
    nk_label(ctx, "webOS version", NK_TEXT_LEFT);
    nk_label(ctx, webos_release, NK_TEXT_RIGHT);
#if HAVE_SDL
    if (webos_panel_info.w && webos_panel_info.h)
    {
        nk_label(ctx, "Panel resolution", NK_TEXT_LEFT);
        nk_labelf(ctx, NK_TEXT_RIGHT, "%d * %d", webos_panel_info.w, webos_panel_info.h);
    }
    if (webos_panel_info.rate)
    {
        nk_label(ctx, "Refresh rate", NK_TEXT_LEFT);
        nk_labelf(ctx, NK_TEXT_RIGHT, "%d", webos_panel_info.rate);
    }
#endif
#endif
    return true;
}

static int _itemcount()
{
    return 0;
}

static void _onselect()
{
    _audio_platform = platform_preferred_audio(platform_current);
#if OS_WEBOS
    load_webos_info();
#endif
}

#if OS_WEBOS
void load_webos_info()
{
    webos_os_info_get_release(webos_release, sizeof(webos_release));
#if HAVE_SDL
    SDL_webOSGetPanelResolution(&webos_panel_info.w, &webos_panel_info.h);
    SDL_webOSGetRefreshRate(&webos_panel_info.rate);
#endif
}
#endif
struct settings_pane settings_pane_about = {
    .title = "About",
    .render = _render,
    .navkey = NULL,
    .itemcount = _itemcount,
    .onselect = _onselect,
};
