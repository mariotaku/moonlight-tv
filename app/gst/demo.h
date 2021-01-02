#pragma once

#include <stdbool.h>

#include <gst/gst.h>
#include <gst/app/app.h>

#include <SDL.h>

int gst_demo_initialize();
int gst_demo_finalize();

bool gst_demo_dispatch_event(SDL_Event ev);

bool gst_demo_render_background();