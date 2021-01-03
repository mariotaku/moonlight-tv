#pragma once

#include <stdbool.h>
#include <SDL.h>

#ifndef NK_NUKLEAR_H_
#include "nuklear/config.h"
#include "nuklear.h"
#endif

#include "backend/computer_manager.h"

void applications_window_init(struct nk_context *ctx);

bool applications_window(struct nk_context *ctx, PSERVER_LIST node);

bool applications_window_dispatch_userevent(struct nk_context *ctx, SDL_Event ev);

bool applications_window_dispatch_inputevent(struct nk_context *ctx, SDL_Event ev);