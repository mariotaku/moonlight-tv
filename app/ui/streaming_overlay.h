#pragma once

#include <stdbool.h>

#ifndef NK_NUKLEAR_H_
#include "nuklear/config.h"
#include "nuklear.h"
#endif

#include "backend/computer_manager.h"

void streaming_overlay_init(struct nk_context *ctx);

bool streaming_overlay(struct nk_context *ctx);