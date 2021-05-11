#pragma once

#include "ui/config.h"

#ifndef NK_NUKLEAR_H_
#include "nuklear/config.h"
#include "nuklear.h"
#include "nuklear/ext_functions.h"
#endif

extern struct nk_font *font_ui_15;
extern struct nk_font *font_num_40;

void fonts_init(struct nk_font_atlas *atlas, const char *path);