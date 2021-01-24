#pragma once

#ifndef NK_NUKLEAR_H_
#include "nuklear.h"
#endif


struct nk_spritesheet_ui {
    struct nk_image ic_action_refresh;
    struct nk_image ic_action_settings;
};

void nk_spritesheet_init_ui_1x(struct nk_image sprites, struct nk_spritesheet_ui *sheet);
void nk_spritesheet_init_ui_2x(struct nk_image sprites, struct nk_spritesheet_ui *sheet);
void nk_spritesheet_init_ui_3x(struct nk_image sprites, struct nk_spritesheet_ui *sheet);


#ifdef NK_IMPLEMENTATION

void nk_spritesheet_init_ui_1x(struct nk_image sprites, struct nk_spritesheet_ui *sheet)
{
    sheet->ic_action_refresh = nk_subimage_handle(sprites.handle, 64, 32, nk_rect(0, 0, 32, 32));
    sheet->ic_action_settings = nk_subimage_handle(sprites.handle, 64, 32, nk_rect(32, 0, 32, 32));
}

void nk_spritesheet_init_ui_2x(struct nk_image sprites, struct nk_spritesheet_ui *sheet)
{
    sheet->ic_action_refresh = nk_subimage_handle(sprites.handle, 128, 64, nk_rect(0, 0, 64, 64));
    sheet->ic_action_settings = nk_subimage_handle(sprites.handle, 128, 64, nk_rect(64, 0, 64, 64));
}

void nk_spritesheet_init_ui_3x(struct nk_image sprites, struct nk_spritesheet_ui *sheet)
{
    sheet->ic_action_refresh = nk_subimage_handle(sprites.handle, 192, 96, nk_rect(0, 0, 96, 96));
    sheet->ic_action_settings = nk_subimage_handle(sprites.handle, 192, 96, nk_rect(96, 0, 96, 96));
}

#endif
