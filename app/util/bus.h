#pragma once

#include <stdbool.h>

#define BUS_INT_EVENT_ACTION 99
#define BUS_EVENT_START 100

typedef void(*bus_actionfunc)(void *);

bool bus_pushevent(int which, void *data1, void *data2);

bool bus_pushaction(bus_actionfunc action, void *data);