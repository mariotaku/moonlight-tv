#include "session_priv.h"

void session_set_state(session_t *session, STREAMING_STATE state) {
    SDL_LockMutex(session->state_lock);
    session->state = state;
    SDL_UnlockMutex(session->state_lock);
}
