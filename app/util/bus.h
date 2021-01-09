#pragma once

#include <stdbool.h>

void bus_init();

void bus_destroy();

bool bus_pushevent(int which, void *data1, void *data2);

bool bus_pollevent(int *which, void **data1, void **data2);