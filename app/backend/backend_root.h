#pragma once

#include <stdbool.h>

void backend_init();

void backend_destroy();

bool backend_dispatch_userevent(int which, void *data1, void *data2);