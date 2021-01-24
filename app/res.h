#pragma once

#define INCBIN_PREFIX res_
#define INCBIN_STYLE INCBIN_STYLE_SNAKE
#include <incbin.h>

#ifdef RES_IMPL
INCBIN(notosans_regular, "../res/NotoSans-Regular.ttf");
INCBIN(default_cover, "../res/defcover.png");
INCBIN(spritesheet_ui_2x, "../res/spritesheet_ui@2x.png");
INCBIN(spritesheet_ui_3x, "../res/spritesheet_ui@3x.png");
#else
INCBIN_EXTERN(notosans_regular);
INCBIN_EXTERN(default_cover);
INCBIN_EXTERN(spritesheet_ui_2x);
INCBIN_EXTERN(spritesheet_ui_3x);
#endif

#ifndef NK_NUKLEAR_H_
#include "nuklear/config.h"
#include "nuklear.h"
#include "nuklear/spritesheet_ui.h"
#endif

extern struct nk_spritesheet_ui sprites_ui;