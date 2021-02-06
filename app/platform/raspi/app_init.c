#include "app_init.h"
#include "app.h"
#include "stream/settings.h"

#include <sys/mman.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <SDL_syswm.h>

#include <interface/vmcs_host/vc_dispmanx.h>

void app_rpi_window_setup(SDL_Window *window)
{
    if (strcmp("RPI", SDL_GetCurrentVideoDriver()))
    {
        return;
    }
}