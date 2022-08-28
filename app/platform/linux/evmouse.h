#pragma once

typedef struct evmouse_t evmouse_t;

evmouse_t *evmouse_open_default();

void evmouse_close(evmouse_t *mouse);