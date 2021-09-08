#include "backend_root.h"

#include "pcmanager.h"
#include "input_manager.h"
#include "stream/session.h"

pcmanager_t *pcmanager;

void backend_init() {
    pcmanager = pcmanager_new();
    streaming_init();
    inputmgr_init();
}

void backend_destroy() {
    inputmgr_destroy();
    streaming_destroy();
    pcmanager_destroy(pcmanager);
}

bool backend_dispatch_userevent(int which, void *data1, void *data2) {
    bool handled = false;
    return handled;
}