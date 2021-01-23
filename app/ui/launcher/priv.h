#pragma once

#include <stdbool.h>

#include "backend/computer_manager.h"

void _select_computer(PSERVER_LIST node, bool load_apps);

void _open_pair(int index, PSERVER_LIST node);