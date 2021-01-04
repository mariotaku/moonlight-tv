#pragma once

#include <stdbool.h>

#ifndef NK_NUKLEAR_H_
#include "nuklear/config.h"
#include "nuklear.h"
#include "nuklear/ext_widgets.h"
#endif

#include "backend/computer_manager.h"

void launcher_window_init(struct nk_context *ctx);

bool launcher_window(struct nk_context *ctx);

bool cw_application_list(struct nk_context *ctx, PSERVER_LIST node, bool event_emitted);