#pragma once

#include <stdbool.h>
#include <string.h>

#include "ui/config.h"

#ifndef NK_NUKLEAR_H_
#include "nuklear/config.h"
#include "nuklear.h"
#include "nuklear/ext_functions.h"
#include "nuklear/ext_image.h"
#include "nuklear/ext_text.h"
#include "nuklear/ext_text_multiline.h"
#include "nuklear/ext_dialog.h"
#include "nuklear/ext_sprites.h"
#include "nuklear/ext_imgview.h"
#include "nuklear/platform_sprites.h"
#endif

#include "backend/computer_manager.h"
#include "stream/session.h"

#include "util/navkey.h"

extern bool stream_overlay_showing;

void streaming_overlay_init(struct nk_context *ctx);

bool streaming_overlay(struct nk_context *ctx, STREAMING_STATUS stat);

bool streaming_overlay_dispatch_userevent(int which);

bool streaming_overlay_dispatch_navkey(struct nk_context *ctx, NAVKEY navkey, NAVKEY_STATE state);

bool streaming_overlay_should_block_input();

bool streaming_overlay_hide();

bool streaming_overlay_show();