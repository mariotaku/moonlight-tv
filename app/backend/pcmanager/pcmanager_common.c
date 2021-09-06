#include "priv.h"
#include <SDL.h>

void invoke_callback(invoke_callback_t *args) {
    args->callback(args->resp, args->userdata);
    SDL_free(args);
}

invoke_callback_t *invoke_callback_args(PPCMANAGER_RESP resp, pcmanager_callback_t callback, void *userdata) {
    invoke_callback_t *args = SDL_malloc(sizeof(invoke_callback_t));
    args->resp = resp;
    args->callback = callback;
    args->userdata = userdata;
    return args;
}