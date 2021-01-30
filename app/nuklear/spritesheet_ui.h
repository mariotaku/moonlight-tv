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
    struct nk_image ic_gamepad_a;
    struct nk_image ic_gamepad_b;
    struct nk_image ic_gamepad_view;
    struct nk_image ic_gamepad_x;
    struct nk_image ic_hourglass;
    struct nk_image ic_play;
    struct nk_image ic_refresh;
    struct nk_image ic_remote_back;
    struct nk_image ic_remote_blue;
    struct nk_image ic_remote_center;
    struct nk_image ic_remote_green;
    struct nk_image ic_remote_red;
    struct nk_image ic_remote_yellow;
    struct nk_image ic_settings;
};

void nk_spritesheet_init_ui_1x(struct nk_image sprites, struct nk_spritesheet_ui *sheet);
void nk_spritesheet_init_ui_2x(struct nk_image sprites, struct nk_spritesheet_ui *sheet);
void nk_spritesheet_init_ui_3x(struct nk_image sprites, struct nk_spritesheet_ui *sheet);


#ifdef NK_IMPLEMENTATION

void nk_spritesheet_init_ui_1x(struct nk_image sprites, struct nk_spritesheet_ui *sheet)
{
    sheet->ic_check_box = nk_subimage_handle(sprites.handle, 128, 128, nk_rect(0, 0, 24, 24));
    sheet->ic_check_box_blank = nk_subimage_handle(sprites.handle, 128, 128, nk_rect(24, 0, 24, 24));
    sheet->ic_check_box_blank_hovered = nk_subimage_handle(sprites.handle, 128, 128, nk_rect(0, 24, 24, 24));
    sheet->ic_check_box_hovered = nk_subimage_handle(sprites.handle, 128, 128, nk_rect(24, 24, 24, 24));
    sheet->ic_close = nk_subimage_handle(sprites.handle, 128, 128, nk_rect(48, 0, 18, 18));
    sheet->ic_gamepad = nk_subimage_handle(sprites.handle, 128, 128, nk_rect(48, 18, 18, 18));
    sheet->ic_gamepad_a = nk_subimage_handle(sprites.handle, 128, 128, nk_rect(0, 48, 16, 16));
    sheet->ic_gamepad_b = nk_subimage_handle(sprites.handle, 128, 128, nk_rect(16, 48, 16, 16));
    sheet->ic_gamepad_view = nk_subimage_handle(sprites.handle, 128, 128, nk_rect(32, 48, 16, 16));
    sheet->ic_gamepad_x = nk_subimage_handle(sprites.handle, 128, 128, nk_rect(48, 36, 16, 16));
    sheet->ic_hourglass = nk_subimage_handle(sprites.handle, 128, 128, nk_rect(64, 36, 24, 24));
    sheet->ic_play = nk_subimage_handle(sprites.handle, 128, 128, nk_rect(66, 0, 18, 18));
    sheet->ic_refresh = nk_subimage_handle(sprites.handle, 128, 128, nk_rect(84, 0, 24, 24));
    sheet->ic_remote_back = nk_subimage_handle(sprites.handle, 128, 128, nk_rect(66, 18, 16, 16));
    sheet->ic_remote_blue = nk_subimage_handle(sprites.handle, 128, 128, nk_rect(108, 0, 16, 16));
    sheet->ic_remote_center = nk_subimage_handle(sprites.handle, 128, 128, nk_rect(108, 16, 16, 16));
    sheet->ic_remote_green = nk_subimage_handle(sprites.handle, 128, 128, nk_rect(88, 24, 16, 16));
    sheet->ic_remote_red = nk_subimage_handle(sprites.handle, 128, 128, nk_rect(88, 40, 16, 16));
    sheet->ic_remote_yellow = nk_subimage_handle(sprites.handle, 128, 128, nk_rect(104, 32, 16, 16));
    sheet->ic_settings = nk_subimage_handle(sprites.handle, 128, 128, nk_rect(104, 48, 24, 24));
}

void nk_spritesheet_init_ui_2x(struct nk_image sprites, struct nk_spritesheet_ui *sheet)
{
    sheet->ic_check_box = nk_subimage_handle(sprites.handle, 256, 256, nk_rect(0, 0, 48, 48));
    sheet->ic_check_box_blank = nk_subimage_handle(sprites.handle, 256, 256, nk_rect(48, 0, 48, 48));
    sheet->ic_check_box_blank_hovered = nk_subimage_handle(sprites.handle, 256, 256, nk_rect(0, 48, 48, 48));
    sheet->ic_check_box_hovered = nk_subimage_handle(sprites.handle, 256, 256, nk_rect(48, 48, 48, 48));
    sheet->ic_close = nk_subimage_handle(sprites.handle, 256, 256, nk_rect(96, 0, 36, 36));
    sheet->ic_gamepad = nk_subimage_handle(sprites.handle, 256, 256, nk_rect(96, 36, 36, 36));
    sheet->ic_gamepad_a = nk_subimage_handle(sprites.handle, 256, 256, nk_rect(0, 96, 32, 32));
    sheet->ic_gamepad_b = nk_subimage_handle(sprites.handle, 256, 256, nk_rect(32, 96, 32, 32));
    sheet->ic_gamepad_view = nk_subimage_handle(sprites.handle, 256, 256, nk_rect(64, 96, 32, 32));
    sheet->ic_gamepad_x = nk_subimage_handle(sprites.handle, 256, 256, nk_rect(96, 72, 32, 32));
    sheet->ic_hourglass = nk_subimage_handle(sprites.handle, 256, 256, nk_rect(128, 72, 48, 48));
    sheet->ic_play = nk_subimage_handle(sprites.handle, 256, 256, nk_rect(132, 0, 36, 36));
    sheet->ic_refresh = nk_subimage_handle(sprites.handle, 256, 256, nk_rect(168, 0, 48, 48));
    sheet->ic_remote_back = nk_subimage_handle(sprites.handle, 256, 256, nk_rect(132, 36, 32, 32));
    sheet->ic_remote_blue = nk_subimage_handle(sprites.handle, 256, 256, nk_rect(216, 0, 32, 32));
    sheet->ic_remote_center = nk_subimage_handle(sprites.handle, 256, 256, nk_rect(216, 32, 32, 32));
    sheet->ic_remote_green = nk_subimage_handle(sprites.handle, 256, 256, nk_rect(176, 48, 32, 32));
    sheet->ic_remote_red = nk_subimage_handle(sprites.handle, 256, 256, nk_rect(176, 80, 32, 32));
    sheet->ic_remote_yellow = nk_subimage_handle(sprites.handle, 256, 256, nk_rect(208, 64, 32, 32));
    sheet->ic_settings = nk_subimage_handle(sprites.handle, 256, 256, nk_rect(208, 96, 48, 48));
}

void nk_spritesheet_init_ui_3x(struct nk_image sprites, struct nk_spritesheet_ui *sheet)
{
    sheet->ic_check_box = nk_subimage_handle(sprites.handle, 384, 384, nk_rect(0, 0, 72, 72));
    sheet->ic_check_box_blank = nk_subimage_handle(sprites.handle, 384, 384, nk_rect(72, 0, 72, 72));
    sheet->ic_check_box_blank_hovered = nk_subimage_handle(sprites.handle, 384, 384, nk_rect(0, 72, 72, 72));
    sheet->ic_check_box_hovered = nk_subimage_handle(sprites.handle, 384, 384, nk_rect(72, 72, 72, 72));
    sheet->ic_close = nk_subimage_handle(sprites.handle, 384, 384, nk_rect(144, 0, 54, 54));
    sheet->ic_gamepad = nk_subimage_handle(sprites.handle, 384, 384, nk_rect(144, 54, 54, 54));
    sheet->ic_gamepad_a = nk_subimage_handle(sprites.handle, 384, 384, nk_rect(0, 144, 48, 48));
    sheet->ic_gamepad_b = nk_subimage_handle(sprites.handle, 384, 384, nk_rect(48, 144, 48, 48));
    sheet->ic_gamepad_view = nk_subimage_handle(sprites.handle, 384, 384, nk_rect(96, 144, 48, 48));
    sheet->ic_gamepad_x = nk_subimage_handle(sprites.handle, 384, 384, nk_rect(144, 108, 48, 48));
    sheet->ic_hourglass = nk_subimage_handle(sprites.handle, 384, 384, nk_rect(192, 108, 72, 72));
    sheet->ic_play = nk_subimage_handle(sprites.handle, 384, 384, nk_rect(198, 0, 54, 54));
    sheet->ic_refresh = nk_subimage_handle(sprites.handle, 384, 384, nk_rect(252, 0, 72, 72));
    sheet->ic_remote_back = nk_subimage_handle(sprites.handle, 384, 384, nk_rect(198, 54, 48, 48));
    sheet->ic_remote_blue = nk_subimage_handle(sprites.handle, 384, 384, nk_rect(324, 0, 48, 48));
    sheet->ic_remote_center = nk_subimage_handle(sprites.handle, 384, 384, nk_rect(324, 48, 48, 48));
    sheet->ic_remote_green = nk_subimage_handle(sprites.handle, 384, 384, nk_rect(264, 72, 48, 48));
    sheet->ic_remote_red = nk_subimage_handle(sprites.handle, 384, 384, nk_rect(264, 120, 48, 48));
    sheet->ic_remote_yellow = nk_subimage_handle(sprites.handle, 384, 384, nk_rect(312, 96, 48, 48));
    sheet->ic_settings = nk_subimage_handle(sprites.handle, 384, 384, nk_rect(312, 144, 72, 72));
}

#endif
