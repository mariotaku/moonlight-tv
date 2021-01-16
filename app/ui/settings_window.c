#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "stream/settings.h"
#include "settings_window.h"
#include "gui_root.h"
#include "app.h"

#define WINDOW_TITLE "Settings"

struct _resolution_option
{
    int w, h;
    char name[8];
};

static const struct _resolution_option _supported_resolutions[] = {
    {1280, 720, "720P\0"},
    {1920, 1080, "1080P\0"},
    {2560, 1440, "2K\0"},
    {3840, 2160, "4K\0"},
};
#define _supported_resolutions_len sizeof(_supported_resolutions) / sizeof(struct _resolution_option)

struct _fps_option
{
    int fps;
    char name[8];
};

static const struct _fps_option _supported_fps[] = {
    {30, "30 FPS"},
    {60, "60 FPS"},
    {120, "120 FPS"},
};
#define _supported_fps_len sizeof(_supported_fps) / sizeof(struct _fps_option)

static char _res_label[8], _fps_label[8];

static void _set_fps(int fps);
static void _set_res(int w, int h);
static void _update_bitrate();

void settings_window_init(struct nk_context *ctx)
{
}

bool settings_window_open()
{
    if (gui_settings_showing)
    {
        return false;
    }
    gui_settings_showing = true;
    _set_fps(app_configuration->stream.fps);
    _set_res(app_configuration->stream.width, app_configuration->stream.height);
    return true;
}

bool settings_window_close()
{
    if (!gui_settings_showing)
    {
        return false;
    }
    gui_settings_showing = false;
    settings_save(app_configuration);
    return true;
}

bool settings_window(struct nk_context *ctx)
{
    struct nk_rect s = nk_rect_s_centered(gui_logic_width, gui_logic_height, gui_logic_width - 240, gui_logic_height - 60);
    if (nk_begin(ctx, WINDOW_TITLE, s, NK_WINDOW_BORDER | NK_WINDOW_CLOSABLE | NK_WINDOW_TITLE))
    {
        struct nk_vec2 content_size = nk_window_get_content_inner_size(ctx);
        nk_layout_row_dynamic_s(ctx, 25, 1);
        nk_label(ctx, "Resolution and FPS", NK_TEXT_LEFT);
        static const float ratio_resolution_fps[] = {0.6, 0.4};
        nk_layout_row_s(ctx, NK_DYNAMIC, 25, 2, ratio_resolution_fps);
        if (nk_combo_begin_label(ctx, _res_label, nk_vec2(nk_widget_width(ctx), 200 * NK_UI_SCALE)))
        {
            nk_layout_row_dynamic_s(ctx, 25, 1);
            for (int i = 0; i < _supported_resolutions_len; i++)
            {
                struct _resolution_option o = _supported_resolutions[i];
                if (nk_combo_item_label(ctx, o.name, NK_TEXT_LEFT))
                {
                    _set_res(o.w, o.h);
                    _update_bitrate();
                }
            }
            nk_combo_end(ctx);
        }
        if (nk_combo_begin_label(ctx, _fps_label, nk_vec2(nk_widget_width(ctx), 200 * NK_UI_SCALE)))
        {
            nk_layout_row_dynamic_s(ctx, 25, 1);
            for (int i = 0; i < _supported_fps_len; i++)
            {
                struct _fps_option o = _supported_fps[i];
                if (nk_combo_item_label(ctx, o.name, NK_TEXT_LEFT))
                {
                    _set_fps(o.fps);
                    _update_bitrate();
                }
            }
            nk_combo_end(ctx);
        }
        nk_layout_row_dynamic_s(ctx, 25, 1);
        nk_label(ctx, "Video bitrate", NK_TEXT_LEFT);
        nk_layout_row_dynamic_s(ctx, 25, 1);
        nk_property_int(ctx, "kbps:", 5000, &app_configuration->stream.bitrate, 100000, 500, 50);

        nk_layout_row_dynamic_s(ctx, 4, 1);
        nk_spacing(ctx, 1);
        nk_layout_row_dynamic_s(ctx, 25, 1);
        nk_label(ctx, "Host Settings", NK_TEXT_LEFT);
        nk_layout_row_dynamic_s(ctx, 25, 1);

        nk_bool sops = app_configuration->sops ? nk_true : nk_false;
        nk_checkbox_label(ctx, "Optimize game settings for streaming", &sops);
        app_configuration->sops = sops == nk_true;

        nk_bool localaudio = app_configuration->localaudio ? nk_true : nk_false;
        nk_checkbox_label(ctx, "Play audio on host PC", &localaudio);
        app_configuration->localaudio = localaudio == nk_true;

        nk_bool quitappafter = app_configuration->quitappafter ? nk_true : nk_false;
        nk_checkbox_label(ctx, "Quit app on host PC after ending stream", &quitappafter);
        app_configuration->quitappafter = quitappafter == nk_true;

        nk_bool viewonly = app_configuration->viewonly ? nk_true : nk_false;
        nk_checkbox_label(ctx, "Disable all input processing (view-only mode)", &viewonly);
        app_configuration->viewonly = viewonly == nk_true;

#if OS_WEBOS
        nk_layout_row_dynamic_s(ctx, 4, 1);
        nk_spacing(ctx, 1);
        nk_layout_row_dynamic_s(ctx, 25, 1);
        nk_label(ctx, "Audio/Video Decoder", NK_TEXT_LEFT);
        nk_layout_row_dynamic_s(ctx, 25, 1);
        static const char *platforms[] = {"auto", "legacy"};
        if (nk_combo_begin_label(ctx, app_configuration->platform, nk_vec2(nk_widget_width(ctx), 200 * NK_UI_SCALE)))
        {
            nk_layout_row_dynamic_s(ctx, 25, 1);
            for (int i = 0; i < NK_LEN(platforms); i++)
            {
                if (nk_combo_item_label(ctx, platforms[i], NK_TEXT_LEFT))
                {
                    app_configuration->platform = (char *)platforms[i];
                }
            }
            nk_combo_end(ctx);
        }
        nk_layout_row_dynamic_s(ctx, 50, 1);
        nk_spacing(ctx, 1);
#endif
    }
    nk_end(ctx);
    // Why Nuklear why, the button looks like "close" but it actually "hide"
    if (nk_window_is_hidden(ctx, WINDOW_TITLE))
    {
        nk_window_close(ctx, WINDOW_TITLE);
        return false;
    }
    return true;
}

bool settings_window_dispatch_navkey(struct nk_context *ctx, NAVKEY navkey)
{
    switch (navkey)
    {
    case NAVKEY_BACK:
        nk_window_show(ctx, WINDOW_TITLE, false);
        break;

    default:
        break;
    }
    return true;
}

void _set_fps(int fps)
{
    sprintf(_fps_label, "%3d FPS", fps);
    app_configuration->stream.fps = fps;
}

void _set_res(int w, int h)
{
    switch (RES_MERGE(w, h))
    {
    case RES_720P:
        sprintf(_res_label, "720P");
        break;
    case RES_1080P:
        sprintf(_res_label, "1080P");
        break;
    case RES_2K:
        sprintf(_res_label, "2K");
        break;
    case RES_4K:
        sprintf(_res_label, "4K");
        break;
    default:
        sprintf(_res_label, "%3d*%3d", w, h);
        break;
    }
    app_configuration->stream.width = w;
    app_configuration->stream.height = h;
}

void _update_bitrate()
{
    app_configuration->stream.bitrate = settings_optimal_bitrate(app_configuration->stream.width, app_configuration->stream.height, app_configuration->stream.fps);
}