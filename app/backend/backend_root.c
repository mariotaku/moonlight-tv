#include "backend_root.h"

#include "pcmanager.h"
#include "input_manager.h"
#include "stream/session.h"

pcmanager_t *pcmanager;

void backend_init(app_backend_t *backend) {
    pcmanager = pcmanager_new();
    streaming_init();
    inputmgr_init(&backend->input_manager);
}

void backend_destroy(app_backend_t *backend) {
    inputmgr_deinit(&backend->input_manager);
    streaming_destroy();
    pcmanager_destroy(pcmanager);
}

bool backend_dispatch_userevent(int which, void *data1, void *data2) {
    bool handled = false;
    return handled;
}