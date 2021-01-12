#pragma once

#include <stdbool.h>

#include "ui/config.h"

#ifndef NK_NUKLEAR_H_
#include "nuklear/config.h"
#include "nuklear.h"
#include "nuklear/ext_widgets.h"
#include "nuklear/ext_functions.h"
#endif

#include "backend/computer_manager.h"

void launcher_window_init(struct nk_context *ctx);

void launcher_display_size(struct nk_context *ctx, short width, short height);

bool launcher_window(struct nk_context *ctx);

bool launcher_applist(struct nk_context *ctx, PSERVER_LIST node, bool event_emitted);