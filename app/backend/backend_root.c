#include "backend_root.h"

#include "computer_manager.h"
#include "application_manager.h"
#include "streaming_session.h"

void backend_init()
{
    computer_manager_init();
    application_manager_init();
    streaming_init();
}

void backend_destroy()
{
    streaming_destroy();
    application_manager_destroy();
    computer_manager_destroy();
}

bool backend_dispatch_event(SDL_Event ev)
{
    return application_manager_dispatch_event(ev);
}