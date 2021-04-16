#pragma once
#include "stream/api.h"

#include <stdbool.h>

#define TVService_SetLowDelayMode PLUGIN_SYMBOL_NAME(TVService_SetLowDelayMode)

bool TVService_SetLowDelayMode(bool value);