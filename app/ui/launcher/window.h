#pragma once

#include <stdbool.h>
#include <ui/manager.h>

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
#include "nuklear/ext_styling.h"
#include "nuklear/ext_imgview.h"
#include "nuklear/ext_smooth_list_view.h"
#include "nuklear/platform_sprites.h"
#endif
#include "lvgl.h"

#include "util/navkey.h"

#include "backend/computer_manager.h"

extern struct nk_image launcher_default_cover;
extern PSERVER_LIST selected_server_node;
extern PAPP_DLIST applist_hovered_item;
extern bool pclist_showing;

bool launcher_applist(struct nk_context *ctx, PSERVER_LIST node, bool event_emitted);


ui_view_controller_t *launcher_controller(const void *args);