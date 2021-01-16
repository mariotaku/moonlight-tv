#pragma once

#include <stdbool.h>

#include "ui/config.h"
#include "util/navkey.h"

#ifndef NK_NUKLEAR_H_
#include "nuklear/config.h"
#include "nuklear.h"
#include "nuklear/ext_functions.h"
#endif

void settings_window_init(struct nk_context *ctx);

bool settings_window_open();

bool settings_window_close();

bool settings_window(struct nk_context *ctx);

bool settings_window_dispatch_navkey(struct nk_context *ctx, NAVKEY navkey);