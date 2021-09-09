#include "util/bus.h"

#include <SDL.h>

bool bus_pushevent(int which, void *data1, void *data2) {
    SDL_Event ev;
    ev.type = SDL_USEREVENT;
    ev.user.code = which;
    ev.user.data1 = data1;
    ev.user.data2 = data2;
    SDL_PushEvent(&ev);
    return true;
}

bool bus_pushaction(bus_actionfunc action, void *data) {
    SDL_assert(action);
    return bus_pushevent(BUS_INT_EVENT_ACTION, action, data);
}