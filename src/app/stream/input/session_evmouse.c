#include "session_evmouse.h"
#include "evmouse.h"
#include "stream/session_priv.h"

#include <errno.h>
#include <assert.h>

#include "logging.h"

static void mouse_listener(const evmouse_event_t *event, void *userdata);

static int mouse_worker(session_evmouse_t *mouse);

static void set_evmouse(session_evmouse_t *mouse, evmouse_t *dev);

void session_evmouse_init(session_evmouse_t *mouse, session_t *session) {
    mouse->session = session;
    mouse->lock = SDL_CreateMutex();
    mouse->cond = SDL_CreateCond();
    mouse->disabled = SDL_FALSE;
    mouse->thread = SDL_CreateThread((SDL_ThreadFunction) mouse_worker, "sessinput", mouse);
}

void session_evmouse_deinit(session_evmouse_t *mouse) {
    if (mouse->thread != NULL) {
        SDL_WaitThread(mouse->thread, NULL);
        mouse->thread = NULL;
    }
    SDL_DestroyMutex(mouse->lock);
    mouse->lock = NULL;
    SDL_DestroyCond(mouse->cond);
    mouse->cond = NULL;
}

void session_evmouse_wait_ready(session_evmouse_t *mouse) {
    SDL_LockMutex(mouse->lock);
    while (mouse->dev == NULL) {
        SDL_CondWait(mouse->cond, mouse->lock);
    }
    SDL_UnlockMutex(mouse->lock);
}

void session_evmouse_interrupt(session_evmouse_t *mouse) {
    SDL_LockMutex(mouse->lock);
    assert (mouse->dev != NULL);
    evmouse_interrupt(mouse->dev);
    SDL_UnlockMutex(mouse->lock);
}

void session_evmouse_disable(session_evmouse_t *mouse) {
    SDL_LockMutex(mouse->lock);
    if (!mouse->disabled) {
        commons_log_info("Session", "EvMouse disable input");
        mouse->disabled = SDL_TRUE;
    }
    SDL_UnlockMutex(mouse->lock);
}

void session_evmouse_enable(session_evmouse_t *mouse) {
    SDL_LockMutex(mouse->lock);
    if (mouse->disabled) {
        commons_log_info("Session", "EvMouse enable input");
        mouse->disabled = SDL_FALSE;
    }
    SDL_UnlockMutex(mouse->lock);
}

static int mouse_worker(session_evmouse_t *mouse) {
    evmouse_t *dev = evmouse_open_default();
    set_evmouse(mouse, dev);
    if (dev == NULL) {
        commons_log_warn("Session", "No mouse device available");
        return ENODEV;
    }
    commons_log_info("Session", "EvMouse opened");
    evmouse_listen(dev, mouse_listener, mouse);
    set_evmouse(mouse, NULL);
    evmouse_close(dev);
    commons_log_info("Session", "EvMouse closed");
    return 0;
}

static void set_evmouse(session_evmouse_t *mouse, evmouse_t *dev) {
    SDL_LockMutex(mouse->lock);
    mouse->dev = dev;
    SDL_CondSignal(mouse->cond);
    SDL_UnlockMutex(mouse->lock);
}

static void mouse_listener(const evmouse_event_t *event, void *userdata) {
    session_evmouse_t *mouse = userdata;
    SDL_LockMutex(mouse->lock);
    session_t *session = mouse->session;
    if (!session_accepting_input(session) || mouse->disabled) {
        SDL_UnlockMutex(mouse->lock);
        return;
    }
    switch (event->type) {
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP: {
            commons_log_info("Session", "Mouse button %d %s", event->button.button,
                             event->type == SDL_MOUSEBUTTONDOWN ? "down" : "up");
            stream_input_handle_mbutton(&session->input, &event->button);
            break;
        }
        case SDL_MOUSEMOTION: {
            stream_input_handle_mmotion(&session->input, &event->motion, true);
            break;
        }
        case SDL_MOUSEWHEEL: {
            stream_input_handle_mwheel(&session->input, &event->wheel);
            break;
        }
    }
    SDL_UnlockMutex(mouse->lock);
}