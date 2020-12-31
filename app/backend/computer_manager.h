#pragma once

#include <glib.h>

#include "computer.h"

static struct COMPUTER_MANAGER_T
{
    GList computersList;
} li_computer_manager;

void computer_manager_init();

void computer_manager_destroy();