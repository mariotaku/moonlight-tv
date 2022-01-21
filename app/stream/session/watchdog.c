#include "stream/session.h"
#include "stream/video/delegate.h"

#include <SDL2/SDL.h>

#define CHECK_INTERVAL 100
#define CHECK_TIMEOUT 3000

static SDL_TimerID timer = 0;
static SDL_atomic_t counter;

static Uint32 watchdog_check();

void streaming_watchdog_start() {
    if (timer) return;
    SDL_AtomicSet(&counter, 0);
    timer = SDL_AddTimer(CHECK_INTERVAL, watchdog_check, NULL);
}

void streaming_watchdog_stop() {
    if (!timer) return;
    SDL_RemoveTimer(timer);
    timer = 0;
}

void streaming_watchdog_reset() {
    SDL_AtomicSet(&counter, 0);
}

static Uint32 watchdog_check() {
    int count = SDL_AtomicAdd(&counter, 1);
    // Interrupt the stream if no frame received over timeout
    if (count >= (CHECK_TIMEOUT / CHECK_INTERVAL)) {
        streaming_interrupt(false, STREAMING_INTERRUPT_WATCHDOG);
        return 0;
    }
    vdec_stat_refresh();
    return CHECK_INTERVAL;
}