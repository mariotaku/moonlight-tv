#pragma once

#ifndef NK_NUKLEAR_H_
#include "nuklear/config.h"
#include "nuklear.h"
#endif

#include "backend/computer_manager.h"

void computers_window_init(struct nk_context *ctx);

void computers_window(struct nk_context *ctx);