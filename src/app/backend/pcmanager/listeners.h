#pragma once

#include "../pcmanager.h"

void pcmanager_listeners_notify(pcmanager_t *manager, const uuidstr_t *uuid, pcmanager_notify_type_t type);