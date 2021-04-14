#pragma once

#include <string.h>

#ifndef NK_NUKLEAR_H_
#include "nuklear/config.h"
#include "nuklear.h"
#endif
#include "nuklear/spritesheet_ui.h"

struct nk_spritesheet_ui sprites_ui;

void nk_ext_sprites_init();
void nk_ext_sprites_destroy();

#ifdef NK_IMPLEMENTATION

static struct nk_image *ui_spritesheet = NULL;
static int ui_spritesheet_density = 0;
struct nk_spritesheet_ui sprites_ui;

void nk_ext_sprites_init()
{
    int density = 0;
    if (NK_UI_SCALE > 2)
        density = 3;
    else if (NK_UI_SCALE > 1)
        density = 2;
    else
        density = 1;
    if (ui_spritesheet_density == density && ui_spritesheet)
        return;
    if (ui_spritesheet)
        nk_ext_sprites_destroy();
    printf("Loading %dx spritesheet\n", density);
    ui_spritesheet = malloc(sizeof(struct nk_image));
    switch (density)
    {
    case 3:
        nk_imageloadm(res_spritesheet_ui_3x_data, res_spritesheet_ui_3x_size, ui_spritesheet);
        nk_image2texture(ui_spritesheet, 0);
        nk_spritesheet_init_ui_3x(*ui_spritesheet, &sprites_ui);
        break;
    case 2:
        nk_imageloadm(res_spritesheet_ui_2x_data, res_spritesheet_ui_2x_size, ui_spritesheet);
        nk_image2texture(ui_spritesheet, 0);
        nk_spritesheet_init_ui_2x(*ui_spritesheet, &sprites_ui);
    default:
        nk_imageloadm(res_spritesheet_ui_1x_data, res_spritesheet_ui_1x_size, ui_spritesheet);
        nk_image2texture(ui_spritesheet, 0);
        nk_spritesheet_init_ui_1x(*ui_spritesheet, &sprites_ui);
        break;
    }
    ui_spritesheet_density = density;
}

void nk_ext_sprites_destroy()
{
    if (!ui_spritesheet)
        return;
    nk_imagetexturefree(ui_spritesheet);
    free(ui_spritesheet);
    ui_spritesheet = NULL;
    ui_spritesheet_density = 0;
}
#endif