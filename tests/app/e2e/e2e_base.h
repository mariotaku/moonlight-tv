#pragma once

#include "unity.h"
#include "app.h"

extern app_t app;

void initSettings(app_settings_t *settings);

void waitFor(int ms);

void fakeTap(int x, int y);

void fakeKeyPress(SDL_Keycode key);