#include "demo.h"

#include <SDL.h>

int gst_demo_initialize()
{
  return 0;
}

int gst_demo_finalize()
{
    return 0;
}

bool gst_demo_dispatch_event(SDL_Event ev)
{
    return false;
}

bool gst_demo_render_background()
{
    return false;
}