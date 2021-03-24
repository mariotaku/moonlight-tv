#include "backend/computer_manager.h"
#include <SDL.h>

static Uint32 _auto_discovery_cb(Uint32 interval, void *repeat);
static SDL_TimerID _auto_discovery_timer = 0;

void computer_manager_auto_discovery_start()
{
    _auto_discovery_timer = SDL_AddTimer(30000, _auto_discovery_cb, (void *)1);
}

void computer_manager_auto_discovery_stop()
{
    if (_auto_discovery_timer)
    {
        SDL_RemoveTimer(_auto_discovery_timer);
    }
}

void computer_manager_auto_discovery_schedule(unsigned int ms)
{
    SDL_AddTimer(ms, _auto_discovery_cb, (void *)0);
}

Uint32 _auto_discovery_cb(Uint32 interval, void *repeat)
{
    computer_manager_run_scan();
    return repeat ? interval : 0;
}