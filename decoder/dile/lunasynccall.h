#pragma once
#include "stream/api.h"

#include <lunaservice.h>

#define LSSyncCallInit DECODER_SYMBOL_NAME(LSSyncCallInit)
#define LSSyncCallbackObtain DECODER_SYMBOL_NAME(LSSyncCallbackObtain)
#define LSSyncCallbackUnlock DECODER_SYMBOL_NAME(LSSyncCallbackUnlock)
#define LSWaitForMessage DECODER_SYMBOL_NAME(LSWaitForMessage)

bool LSSyncCallInit();

LSFilterFunc LSSyncCallbackObtain();

void LSSyncCallbackUnlock();

LSMessage *LSWaitForMessage();