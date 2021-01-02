#pragma once

#include <stdbool.h>

#ifndef NK_NUKLEAR_H_
#include "nuklear/config.h"
#include "nuklear.h"
#endif

#include "backend/computer_manager.h"

void applications_window_init(struct nk_context *ctx);

bool applications_window(struct nk_context *ctx, const char *selected_address);