#pragma once

#include <stdbool.h>

#include "ui/config.h"
#include "util/navkey.h"

#ifndef NK_NUKLEAR_H_
#include "nuklear/config.h"
#include "nuklear.h"
#include "nuklear/ext_functions.h"
#include "nuklear/ext_image.h"
#include "nuklear/ext_text.h"
#include "nuklear/ext_sprites.h"
#include "nuklear/ext_imgview.h"
#include "nuklear/platform_sprites.h"
#endif

#include "stream/settings.h"
#include "app.h"

void settings_window_init(struct nk_context *ctx);

bool settings_window_open();

bool settings_window_close();

bool settings_window(struct nk_context *ctx);

bool settings_window_dispatch_navkey(struct nk_context *ctx, NAVKEY navkey);