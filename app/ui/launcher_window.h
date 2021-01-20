#pragma once

#include <stdbool.h>

#include "ui/config.h"

#ifndef NK_NUKLEAR_H_
#include "nuklear/config.h"
#include "nuklear.h"
#include "nuklear/ext_functions.h"
#include "nuklear/ext_image.h"
#include "nuklear/ext_styling.h"
#endif

#include "util/navkey.h"

#include "backend/computer_manager.h"

extern struct nk_image launcher_default_cover;

void launcher_window_init(struct nk_context *ctx);

void launcher_window_destroy();

void launcher_display_size(struct nk_context *ctx, short width, short height);

bool launcher_window(struct nk_context *ctx);

bool launcher_applist(struct nk_context *ctx, PSERVER_LIST node, bool event_emitted);

bool launcher_window_dispatch_userevent(int which, void *data1, void *data2);

bool launcher_window_dispatch_navkey(struct nk_context *ctx, NAVKEY navkey);