#pragma once

#include <stdbool.h>

#define BUS_INT_EVENT_ACTION 99
#define BUS_EVENT_START 100

typedef struct app_t app_t;

typedef void(*bus_actionfunc)(void *, app_t *app);

bool bus_pushevent(int which, void *data1, void *data2);

bool app_bus_post(app_t *app, bus_actionfunc action, void *data);

bool app_bus_post_sync(app_t *app, bus_actionfunc action, void *data);

/**
 * Drain all bus events
 */
void app_bus_drain(app_t *app);