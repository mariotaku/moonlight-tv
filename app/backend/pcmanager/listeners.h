#pragma once

#include "../pcmanager.h"

void pcmanager_listeners_notify(pcmanager_t *manager, const pcmanager_resp_t* resp, pcmanager_notify_type_t type);