#include "window.h"

#include <stddef.h>
#include <stdio.h>

static char _res_label[8], _fps_label[8];

void _settings_pane_host(struct nk_context *ctx)
{
    nk_layout_row_dynamic_s(ctx, 25, 1);
    nk_label(ctx, "Host Settings", NK_TEXT_LEFT);
    nk_layout_row_dynamic_s(ctx, 25, 1);

    int w = app_configuration->stream.width, h = app_configuration->stream.height,
        fps = app_configuration->stream.fps;
    bool sops_supported = settings_sops_supported(w, h, fps);
    nk_bool sops = sops_supported && app_configuration->sops ? nk_true : nk_false;
    nk_checkbox_label(ctx, "Optimize game settings for streaming", &sops);
    if (sops_supported)
    {
        app_configuration->sops = sops == nk_true;
    }
    else
    {
        nk_layout_row_template_begin_s(ctx, 25);
        nk_layout_row_template_push_static_s(ctx, 20);
        nk_layout_row_template_push_variable_s(ctx, 10);
        nk_layout_row_template_end(ctx);
        nk_spacing(ctx, 1);
        nk_labelf_wrap(ctx, "(Not available under %s@%s)", _res_label, _fps_label);
        nk_layout_row_dynamic_s(ctx, 25, 1);
    }

    nk_checkbox_label_std(ctx, "Play audio on host PC", &app_configuration->localaudio);

    nk_checkbox_label_std(ctx, "Disable all input processing (view-only mode)", &app_configuration->viewonly);
}

void _pane_host_open()
{
}