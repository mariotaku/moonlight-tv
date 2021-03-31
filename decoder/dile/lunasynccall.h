#pragma once
#include <lunaservice.h>

bool LSSyncCallInit();

LSFilterFunc LSSyncCallbackObtain();

void LSSyncCallbackUnlock();

LSMessage *LSWaitForMessage();