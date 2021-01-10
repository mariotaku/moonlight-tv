#pragma once

#include <stdbool.h>

#include "ui/config.h"

#ifndef NK_NUKLEAR_H_
#include "nuklear/config.h"
#include "nuklear.h"
#include "nuklear/ext_functions.h"
#endif

#include "backend/computer_manager.h"
#include "stream/session.h"

void streaming_overlay_init(struct nk_context *ctx);

bool streaming_overlay(struct nk_context *ctx, STREAMING_STATUS stat);

bool streaming_overlay_dispatch_userevent(int which);

bool streaming_overlay_should_block_input();