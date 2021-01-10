#pragma once

#define SDL_IS_INPUT_EVENT(evt) (evt.type >= SDL_KEYDOWN && evt.type < SDL_CLIPBOARDUPDATE)