#pragma once

#ifndef NK_NUKLEAR_H_
#include "nuklear.h"
#endif


struct nk_spritesheet_ui {
    struct nk_image ic_add_to_queue;
    struct nk_image ic_check_box;
    struct nk_image ic_check_box_blank;
    struct nk_image ic_check_box_blank_hovered;
    struct nk_image ic_check_box_hovered;
    struct nk_image ic_close;
    struct nk_image ic_desktop_large;
    struct nk_image ic_gamepad;
    struct nk_image ic_gamepad_a;
    struct nk_image ic_gamepad_b;
    struct nk_image ic_gamepad_view;
    struct nk_image ic_gamepad_x;
    struct nk_image ic_gamepad_y;
    struct nk_image ic_hourglass;
    struct nk_image ic_indicator_hdr;
    struct nk_image ic_info;
    struct nk_image ic_play;
    struct nk_image ic_refresh;
    struct nk_image ic_remote_back;
    struct nk_image ic_remote_blue;
    struct nk_image ic_remote_center;
    struct nk_image ic_remote_green;
    struct nk_image ic_remote_red;
    struct nk_image ic_remote_yellow;
    struct nk_image ic_settings;
    struct nk_image ic_warning;
};

void nk_spritesheet_init_ui_1x(struct nk_image sprites, struct nk_spritesheet_ui *sheet);
void nk_spritesheet_init_ui_2x(struct nk_image sprites, struct nk_spritesheet_ui *sheet);
void nk_spritesheet_init_ui_3x(struct nk_image sprites, struct nk_spritesheet_ui *sheet);


#ifdef NK_IMPLEMENTATION

void nk_spritesheet_init_ui_1x(struct nk_image sprites, struct nk_spritesheet_ui *sheet)
{
    sheet->ic_add_to_queue = nk_subimage_handle(sprites.handle, 128, 128, nk_rect(0, 0, 24, 24));
    sheet->ic_check_box = nk_subimage_handle(sprites.handle, 128, 128, nk_rect(24, 0, 24, 24));
    sheet->ic_check_box_blank = nk_subimage_handle(sprites.handle, 128, 128, nk_rect(0, 24, 24, 24));
    sheet->ic_check_box_blank_hovered = nk_subimage_handle(sprites.handle, 128, 128, nk_rect(24, 24, 24, 24));
    sheet->ic_check_box_hovered = nk_subimage_handle(sprites.handle, 128, 128, nk_rect(48, 0, 24, 24));
    sheet->ic_close = nk_subimage_handle(sprites.handle, 128, 128, nk_rect(48, 24, 18, 18));
    sheet->ic_desktop_large = nk_subimage_handle(sprites.handle, 128, 128, nk_rect(72, 0, 48, 48));
    sheet->ic_gamepad = nk_subimage_handle(sprites.handle, 128, 128, nk_rect(48, 42, 18, 18));
    sheet->ic_gamepad_a = nk_subimage_handle(sprites.handle, 128, 128, nk_rect(0, 48, 16, 16));
    sheet->ic_gamepad_b = nk_subimage_handle(sprites.handle, 128, 128, nk_rect(66, 48, 16, 16));
    sheet->ic_gamepad_view = nk_subimage_handle(sprites.handle, 128, 128, nk_rect(16, 48, 16, 16));
    sheet->ic_gamepad_x = nk_subimage_handle(sprites.handle, 128, 128, nk_rect(82, 48, 16, 16));
    sheet->ic_gamepad_y = nk_subimage_handle(sprites.handle, 128, 128, nk_rect(32, 48, 16, 16));
    sheet->ic_hourglass = nk_subimage_handle(sprites.handle, 128, 128, nk_rect(98, 48, 24, 24));
    sheet->ic_indicator_hdr = nk_subimage_handle(sprites.handle, 128, 128, nk_rect(48, 60, 18, 12));
    sheet->ic_info = nk_subimage_handle(sprites.handle, 128, 128, nk_rect(66, 64, 18, 18));
    sheet->ic_play = nk_subimage_handle(sprites.handle, 128, 128, nk_rect(84, 72, 18, 18));
    sheet->ic_refresh = nk_subimage_handle(sprites.handle, 128, 128, nk_rect(102, 72, 24, 24));
    sheet->ic_remote_back = nk_subimage_handle(sprites.handle, 128, 128, nk_rect(0, 96, 16, 16));
    sheet->ic_remote_blue = nk_subimage_handle(sprites.handle, 128, 128, nk_rect(0, 112, 16, 16));
    sheet->ic_remote_center = nk_subimage_handle(sprites.handle, 128, 128, nk_rect(0, 72, 16, 16));
    sheet->ic_remote_green = nk_subimage_handle(sprites.handle, 128, 128, nk_rect(16, 64, 16, 16));
    sheet->ic_remote_red = nk_subimage_handle(sprites.handle, 128, 128, nk_rect(32, 64, 16, 16));
    sheet->ic_remote_yellow = nk_subimage_handle(sprites.handle, 128, 128, nk_rect(48, 72, 16, 16));
    sheet->ic_settings = nk_subimage_handle(sprites.handle, 128, 128, nk_rect(16, 96, 24, 24));
    sheet->ic_warning = nk_subimage_handle(sprites.handle, 128, 128, nk_rect(64, 82, 18, 18));
}

void nk_spritesheet_init_ui_2x(struct nk_image sprites, struct nk_spritesheet_ui *sheet)
{
    sheet->ic_add_to_queue = nk_subimage_handle(sprites.handle, 256, 256, nk_rect(0, 0, 48, 48));
    sheet->ic_check_box = nk_subimage_handle(sprites.handle, 256, 256, nk_rect(48, 0, 48, 48));
    sheet->ic_check_box_blank = nk_subimage_handle(sprites.handle, 256, 256, nk_rect(0, 48, 48, 48));
    sheet->ic_check_box_blank_hovered = nk_subimage_handle(sprites.handle, 256, 256, nk_rect(48, 48, 48, 48));
    sheet->ic_check_box_hovered = nk_subimage_handle(sprites.handle, 256, 256, nk_rect(96, 0, 48, 48));
    sheet->ic_close = nk_subimage_handle(sprites.handle, 256, 256, nk_rect(96, 48, 36, 36));
    sheet->ic_desktop_large = nk_subimage_handle(sprites.handle, 256, 256, nk_rect(144, 0, 96, 96));
    sheet->ic_gamepad = nk_subimage_handle(sprites.handle, 256, 256, nk_rect(96, 84, 36, 36));
    sheet->ic_gamepad_a = nk_subimage_handle(sprites.handle, 256, 256, nk_rect(0, 96, 32, 32));
    sheet->ic_gamepad_b = nk_subimage_handle(sprites.handle, 256, 256, nk_rect(132, 96, 32, 32));
    sheet->ic_gamepad_view = nk_subimage_handle(sprites.handle, 256, 256, nk_rect(32, 96, 32, 32));
    sheet->ic_gamepad_x = nk_subimage_handle(sprites.handle, 256, 256, nk_rect(164, 96, 32, 32));
    sheet->ic_gamepad_y = nk_subimage_handle(sprites.handle, 256, 256, nk_rect(64, 96, 32, 32));
    sheet->ic_hourglass = nk_subimage_handle(sprites.handle, 256, 256, nk_rect(196, 96, 48, 48));
    sheet->ic_indicator_hdr = nk_subimage_handle(sprites.handle, 256, 256, nk_rect(96, 120, 36, 24));
    sheet->ic_info = nk_subimage_handle(sprites.handle, 256, 256, nk_rect(132, 128, 36, 36));
    sheet->ic_play = nk_subimage_handle(sprites.handle, 256, 256, nk_rect(168, 144, 36, 36));
    sheet->ic_refresh = nk_subimage_handle(sprites.handle, 256, 256, nk_rect(204, 144, 48, 48));
    sheet->ic_remote_back = nk_subimage_handle(sprites.handle, 256, 256, nk_rect(0, 192, 32, 32));
    sheet->ic_remote_blue = nk_subimage_handle(sprites.handle, 256, 256, nk_rect(0, 224, 32, 32));
    sheet->ic_remote_center = nk_subimage_handle(sprites.handle, 256, 256, nk_rect(0, 144, 32, 32));
    sheet->ic_remote_green = nk_subimage_handle(sprites.handle, 256, 256, nk_rect(32, 128, 32, 32));
    sheet->ic_remote_red = nk_subimage_handle(sprites.handle, 256, 256, nk_rect(64, 128, 32, 32));
    sheet->ic_remote_yellow = nk_subimage_handle(sprites.handle, 256, 256, nk_rect(96, 144, 32, 32));
    sheet->ic_settings = nk_subimage_handle(sprites.handle, 256, 256, nk_rect(32, 192, 48, 48));
    sheet->ic_warning = nk_subimage_handle(sprites.handle, 256, 256, nk_rect(128, 164, 36, 36));
}

void nk_spritesheet_init_ui_3x(struct nk_image sprites, struct nk_spritesheet_ui *sheet)
{
    sheet->ic_add_to_queue = nk_subimage_handle(sprites.handle, 384, 384, nk_rect(0, 0, 72, 72));
    sheet->ic_check_box = nk_subimage_handle(sprites.handle, 384, 384, nk_rect(72, 0, 72, 72));
    sheet->ic_check_box_blank = nk_subimage_handle(sprites.handle, 384, 384, nk_rect(0, 72, 72, 72));
    sheet->ic_check_box_blank_hovered = nk_subimage_handle(sprites.handle, 384, 384, nk_rect(72, 72, 72, 72));
    sheet->ic_check_box_hovered = nk_subimage_handle(sprites.handle, 384, 384, nk_rect(144, 0, 72, 72));
    sheet->ic_close = nk_subimage_handle(sprites.handle, 384, 384, nk_rect(144, 72, 54, 54));
    sheet->ic_desktop_large = nk_subimage_handle(sprites.handle, 384, 384, nk_rect(216, 0, 144, 144));
    sheet->ic_gamepad = nk_subimage_handle(sprites.handle, 384, 384, nk_rect(144, 126, 54, 54));
    sheet->ic_gamepad_a = nk_subimage_handle(sprites.handle, 384, 384, nk_rect(0, 144, 48, 48));
    sheet->ic_gamepad_b = nk_subimage_handle(sprites.handle, 384, 384, nk_rect(198, 144, 48, 48));
    sheet->ic_gamepad_view = nk_subimage_handle(sprites.handle, 384, 384, nk_rect(48, 144, 48, 48));
    sheet->ic_gamepad_x = nk_subimage_handle(sprites.handle, 384, 384, nk_rect(246, 144, 48, 48));
    sheet->ic_gamepad_y = nk_subimage_handle(sprites.handle, 384, 384, nk_rect(96, 144, 48, 48));
    sheet->ic_hourglass = nk_subimage_handle(sprites.handle, 384, 384, nk_rect(294, 144, 72, 72));
    sheet->ic_indicator_hdr = nk_subimage_handle(sprites.handle, 384, 384, nk_rect(144, 180, 54, 36));
    sheet->ic_info = nk_subimage_handle(sprites.handle, 384, 384, nk_rect(198, 192, 54, 54));
    sheet->ic_play = nk_subimage_handle(sprites.handle, 384, 384, nk_rect(252, 216, 54, 54));
    sheet->ic_refresh = nk_subimage_handle(sprites.handle, 384, 384, nk_rect(306, 216, 72, 72));
    sheet->ic_remote_back = nk_subimage_handle(sprites.handle, 384, 384, nk_rect(0, 288, 48, 48));
    sheet->ic_remote_blue = nk_subimage_handle(sprites.handle, 384, 384, nk_rect(0, 336, 48, 48));
    sheet->ic_remote_center = nk_subimage_handle(sprites.handle, 384, 384, nk_rect(0, 216, 48, 48));
    sheet->ic_remote_green = nk_subimage_handle(sprites.handle, 384, 384, nk_rect(48, 192, 48, 48));
    sheet->ic_remote_red = nk_subimage_handle(sprites.handle, 384, 384, nk_rect(96, 192, 48, 48));
    sheet->ic_remote_yellow = nk_subimage_handle(sprites.handle, 384, 384, nk_rect(144, 216, 48, 48));
    sheet->ic_settings = nk_subimage_handle(sprites.handle, 384, 384, nk_rect(48, 288, 72, 72));
    sheet->ic_warning = nk_subimage_handle(sprites.handle, 384, 384, nk_rect(192, 246, 54, 54));
}

#endif
