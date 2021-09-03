#include "backend_root.h"

#include "pcmanager.h"
#include "appmanager.h"
#include "input_manager.h"
#include "stream/session.h"
#include "coverloader.h"

void backend_init() {
    coverloader_init();
    computer_manager_init();
    application_manager_init();
    streaming_init();
    inputmgr_init();
}

void backend_destroy() {
    inputmgr_destroy();
    streaming_destroy();
    application_manager_destroy();
    computer_manager_destroy();
    coverloader_destroy();
}

bool backend_dispatch_userevent(int which, void *data1, void *data2) {
    bool handled = false;
    handled |= computer_manager_dispatch_userevent(which, data1, data2);
    handled |= application_manager_dispatch_userevent(which, data1, data2);
    return handled;
}