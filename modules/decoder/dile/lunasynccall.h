#pragma once
#include "stream/module/api.h"

#include <lunaservice.h>

#define LSSyncCallInit PLUGIN_SYMBOL_NAME(LSSyncCallInit)
#define LSSyncCallbackObtain PLUGIN_SYMBOL_NAME(LSSyncCallbackObtain)
#define LSSyncCallbackUnlock PLUGIN_SYMBOL_NAME(LSSyncCallbackUnlock)
#define LSWaitForMessage PLUGIN_SYMBOL_NAME(LSWaitForMessage)

bool LSSyncCallInit();

LSFilterFunc LSSyncCallbackObtain();

void LSSyncCallbackUnlock();

LSMessage *LSWaitForMessage();