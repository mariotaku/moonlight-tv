#pragma once

#ifndef NK_NUKLEAR_H_
#include "nuklear.h"
#endif


struct nk_spritesheet_ui {
    struct nk_image ic_check_box;
    struct nk_image ic_check_box_blank;
    struct nk_image ic_hourglass;
    struct nk_image ic_refresh;
    struct nk_image ic_settings;
};

void nk_spritesheet_init_ui_1x(struct nk_image sprites, struct nk_spritesheet_ui *sheet);
void nk_spritesheet_init_ui_2x(struct nk_image sprites, struct nk_spritesheet_ui *sheet);
void nk_spritesheet_init_ui_3x(struct nk_image sprites, struct nk_spritesheet_ui *sheet);


#ifdef NK_IMPLEMENTATION

void nk_spritesheet_init_ui_1x(struct nk_image sprites, struct nk_spritesheet_ui *sheet)
{
    sheet->ic_check_box = nk_subimage_handle(sprites.handle, 128, 64, nk_rect(0, 0, 24, 24));
    sheet->ic_check_box_blank = nk_subimage_handle(sprites.handle, 128, 64, nk_rect(24, 0, 24, 24));
    sheet->ic_hourglass = nk_subimage_handle(sprites.handle, 128, 64, nk_rect(0, 24, 24, 24));
    sheet->ic_refresh = nk_subimage_handle(sprites.handle, 128, 64, nk_rect(24, 24, 24, 24));
    sheet->ic_settings = nk_subimage_handle(sprites.handle, 128, 64, nk_rect(48, 0, 24, 24));
}

void nk_spritesheet_init_ui_2x(struct nk_image sprites, struct nk_spritesheet_ui *sheet)
{
    sheet->ic_check_box = nk_subimage_handle(sprites.handle, 256, 128, nk_rect(0, 0, 48, 48));
    sheet->ic_check_box_blank = nk_subimage_handle(sprites.handle, 256, 128, nk_rect(48, 0, 48, 48));
    sheet->ic_hourglass = nk_subimage_handle(sprites.handle, 256, 128, nk_rect(0, 48, 48, 48));
    sheet->ic_refresh = nk_subimage_handle(sprites.handle, 256, 128, nk_rect(48, 48, 48, 48));
    sheet->ic_settings = nk_subimage_handle(sprites.handle, 256, 128, nk_rect(96, 0, 48, 48));
}

void nk_spritesheet_init_ui_3x(struct nk_image sprites, struct nk_spritesheet_ui *sheet)
{
    sheet->ic_check_box = nk_subimage_handle(sprites.handle, 384, 192, nk_rect(0, 0, 72, 72));
    sheet->ic_check_box_blank = nk_subimage_handle(sprites.handle, 384, 192, nk_rect(72, 0, 72, 72));
    sheet->ic_hourglass = nk_subimage_handle(sprites.handle, 384, 192, nk_rect(0, 72, 72, 72));
    sheet->ic_refresh = nk_subimage_handle(sprites.handle, 384, 192, nk_rect(72, 72, 72, 72));
    sheet->ic_settings = nk_subimage_handle(sprites.handle, 384, 192, nk_rect(144, 0, 72, 72));
}

#endif
