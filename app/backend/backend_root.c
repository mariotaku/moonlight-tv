#include "backend_root.h"

#include "pcmanager.h"
#include "input_manager.h"
#include "stream/session.h"

void backend_init() {
    computer_manager_init();
    streaming_init();
    inputmgr_init();
}

void backend_destroy() {
    inputmgr_destroy();
    streaming_destroy();
    computer_manager_destroy();
}

bool backend_dispatch_userevent(int which, void *data1, void *data2) {
    bool handled = false;
    return handled;
}