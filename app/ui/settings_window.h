#pragma once

#include <stdbool.h>

#ifndef NK_NUKLEAR_H_
#include "nuklear/config.h"
#include "nuklear.h"
#endif

void settings_window_init(struct nk_context *ctx);

bool settings_window(struct nk_context *ctx);