#include "backend/input_manager.h"
#include "stream/input/absinput.h"

#include <stdio.h>
#include <stdlib.h>

#include "events.h"

void inputmgr_init()
{
    absinput_init();
}

void inputmgr_destroy()
{
    absinput_destroy();
}