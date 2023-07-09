#include "session_evmouse.h"
#include "priv.h"
#include "evmouse.h"
#include <errno.h>

#include "logging.h"
#include "stream/input/sdlinput.h"

static void mouse_listener(const evmouse_event_t *event, void *userdata);

static int mouse_worker(session_t *session);

void session_evmouse_init(session_t *session) {
    session->mouse.thread = SDL_CreateThread((SDL_ThreadFunction) mouse_worker, "sessinput", session);
}

void session_evmouse_deinit(session_t *session) {
    if (session->mouse.thread != NULL) {
        SDL_WaitThread(session->mouse.thread, NULL);
        session->mouse.thread = NULL;
    }
}

void session_evmouse_interrupt(session_t *session) {
    if (session->mouse.dev != NULL) {
        evmouse_interrupt(session->mouse.dev);
    }
}

static int mouse_worker(session_t *session) {
    session->mouse.dev = evmouse_open_default();
    if (session->mouse.dev == NULL) {
        commons_log_warn("Session", "No mouse device available");
        return ENODEV;
    }
    commons_log_info("Session", "EvMouse opened");
    evmouse_listen(session->mouse.dev, mouse_listener, session);
    evmouse_close(session->mouse.dev);
    commons_log_info("Session", "EvMouse closed");
    return 0;
}

static void mouse_listener(const evmouse_event_t *event, void *userdata) {
    session_t *session = userdata;
    if (!session_input_should_accept(&session->input)) {
        return;
    }
    switch (event->type) {
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP: {
            sdlinput_handle_mbutton_event(&event->button);
            break;
        }
        case SDL_MOUSEMOTION:
            LiSendMouseMoveEvent((short) event->motion.xrel, (short) event->motion.yrel);
            break;
        case SDL_MOUSEWHEEL:
            sdlinput_handle_mwheel_event(&event->wheel);
            break;
    }
}