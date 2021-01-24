#pragma once

#ifndef NK_NUKLEAR_H_
#include "nuklear/config.h"
#include "nuklear.h"
#endif
#include "nuklear/spritesheet_ui.h"

extern struct nk_spritesheet_ui sprites_ui;

void nk_ext_sprites_init();
void nk_ext_sprites_destroy();

#ifdef NK_IMPLEMENTATION

static struct nk_image gui_spritesheet;
struct nk_spritesheet_ui sprites_ui;

void nk_ext_sprites_init()
{
    nk_imageloadm(res_spritesheet_ui_3x_data, res_spritesheet_ui_3x_size, &gui_spritesheet);
    nk_image2texture(&gui_spritesheet);
    nk_spritesheet_init_ui_3x(gui_spritesheet, &sprites_ui);
}

void nk_ext_sprites_destroy()
{
    nk_imagetexturefree(&gui_spritesheet);
}
#endif