#pragma once

#include <SDL.h>
#include <stdbool.h>

typedef struct refcounter_t {
    int counter;
    SDL_mutex *lock;
} refcounter_t;

static inline void refcounter_init(refcounter_t *counter) {
    counter->counter = 1;
    counter->lock = SDL_CreateMutex();
}

static inline void refcounter_destroy(refcounter_t *counter) {
    SDL_assert_release(counter->counter == 0);
    SDL_DestroyMutex(counter->lock);
}

static inline void refcounter_ref(refcounter_t *counter) {
    SDL_assert_release(counter->counter > 0);
    SDL_LockMutex(counter->lock);
    counter->counter++;
    SDL_UnlockMutex(counter->lock);
}

static inline bool refcounter_unref(refcounter_t *counter) {
    SDL_LockMutex(counter->lock);
    counter->counter--;
    SDL_UnlockMutex(counter->lock);
    return counter->counter == 0;
}