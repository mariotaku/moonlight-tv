#pragma once

#ifndef NK_NUKLEAR_H_
#include "nuklear.h"
#endif


struct nk_spritesheet_ui {
    struct nk_image ic_check_box;
    struct nk_image ic_check_box_blank;
    struct nk_image ic_check_box_blank_hovered;
    struct nk_image ic_check_box_hovered;
    struct nk_image ic_close;
    struct nk_image ic_gamepad;
    struct nk_image ic_hourglass;
    struct nk_image ic_play;
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
    sheet->ic_check_box_blank_hovered = nk_subimage_handle(sprites.handle, 128, 64, nk_rect(0, 24, 24, 24));
    sheet->ic_check_box_hovered = nk_subimage_handle(sprites.handle, 128, 64, nk_rect(24, 24, 24, 24));
    sheet->ic_close = nk_subimage_handle(sprites.handle, 128, 64, nk_rect(48, 0, 18, 18));
    sheet->ic_gamepad = nk_subimage_handle(sprites.handle, 128, 64, nk_rect(48, 18, 18, 18));
    sheet->ic_hourglass = nk_subimage_handle(sprites.handle, 128, 64, nk_rect(48, 36, 24, 24));
    sheet->ic_play = nk_subimage_handle(sprites.handle, 128, 64, nk_rect(66, 0, 18, 18));
    sheet->ic_refresh = nk_subimage_handle(sprites.handle, 128, 64, nk_rect(84, 0, 24, 24));
    sheet->ic_settings = nk_subimage_handle(sprites.handle, 128, 64, nk_rect(72, 24, 24, 24));
}

void nk_spritesheet_init_ui_2x(struct nk_image sprites, struct nk_spritesheet_ui *sheet)
{
    sheet->ic_check_box = nk_subimage_handle(sprites.handle, 256, 128, nk_rect(0, 0, 48, 48));
    sheet->ic_check_box_blank = nk_subimage_handle(sprites.handle, 256, 128, nk_rect(48, 0, 48, 48));
    sheet->ic_check_box_blank_hovered = nk_subimage_handle(sprites.handle, 256, 128, nk_rect(0, 48, 48, 48));
    sheet->ic_check_box_hovered = nk_subimage_handle(sprites.handle, 256, 128, nk_rect(48, 48, 48, 48));
    sheet->ic_close = nk_subimage_handle(sprites.handle, 256, 128, nk_rect(96, 0, 36, 36));
    sheet->ic_gamepad = nk_subimage_handle(sprites.handle, 256, 128, nk_rect(96, 36, 36, 36));
    sheet->ic_hourglass = nk_subimage_handle(sprites.handle, 256, 128, nk_rect(96, 72, 48, 48));
    sheet->ic_play = nk_subimage_handle(sprites.handle, 256, 128, nk_rect(132, 0, 36, 36));
    sheet->ic_refresh = nk_subimage_handle(sprites.handle, 256, 128, nk_rect(168, 0, 48, 48));
    sheet->ic_settings = nk_subimage_handle(sprites.handle, 256, 128, nk_rect(144, 48, 48, 48));
}

void nk_spritesheet_init_ui_3x(struct nk_image sprites, struct nk_spritesheet_ui *sheet)
{
    sheet->ic_check_box = nk_subimage_handle(sprites.handle, 384, 192, nk_rect(0, 0, 72, 72));
    sheet->ic_check_box_blank = nk_subimage_handle(sprites.handle, 384, 192, nk_rect(72, 0, 72, 72));
    sheet->ic_check_box_blank_hovered = nk_subimage_handle(sprites.handle, 384, 192, nk_rect(0, 72, 72, 72));
    sheet->ic_check_box_hovered = nk_subimage_handle(sprites.handle, 384, 192, nk_rect(72, 72, 72, 72));
    sheet->ic_close = nk_subimage_handle(sprites.handle, 384, 192, nk_rect(144, 0, 54, 54));
    sheet->ic_gamepad = nk_subimage_handle(sprites.handle, 384, 192, nk_rect(144, 54, 54, 54));
    sheet->ic_hourglass = nk_subimage_handle(sprites.handle, 384, 192, nk_rect(144, 108, 72, 72));
    sheet->ic_play = nk_subimage_handle(sprites.handle, 384, 192, nk_rect(198, 0, 54, 54));
    sheet->ic_refresh = nk_subimage_handle(sprites.handle, 384, 192, nk_rect(252, 0, 72, 72));
    sheet->ic_settings = nk_subimage_handle(sprites.handle, 384, 192, nk_rect(216, 72, 72, 72));
}

#endif
