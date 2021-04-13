#pragma once
#include "stream/api.h"

#include <stdbool.h>

#define TVService_SetLowDelayMode DECODER_SYMBOL_NAME(TVService_SetLowDelayMode)

bool TVService_SetLowDelayMode(bool value);