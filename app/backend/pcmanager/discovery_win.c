#include "priv.h"
#include <microdns/microdns.h>
#include "util/logging.h"

static bool stop(void *p_cookie) {
    return false;
}

static void callback(void *p_cookie, int status, const struct rr_entry *entries) {
    char err[128];

    if (status < 0) {
        mdns_strerror(status, err, sizeof(err));
        applog_e("mDNS", "error: %s", err);
        return;
    }

    for (const struct rr_entry *cur = entries; cur; cur = cur->next) {
        switch (cur->type) {
            case RR_A: {
                if (pcmanager_is_known_host(cur->data.A.addr_str)) {
                    break;
                }
                pcmanager_insert_by_address(strdup(cur->data.A.addr_str), false, NULL, NULL);
                break;
            }
        }
    }
}


int _computer_manager_polling_action(void *data) {
    computer_discovery_running = true;
    int r = 0;
    char err[128];
    struct mdns_ctx *ctx = NULL;
    mdns_init(&ctx, NULL, MDNS_PORT);

    static const char *service_name[] = {"_nvstream._tcp.local"};

    if ((r = mdns_init(&ctx, NULL, MDNS_PORT)) < 0)
        goto err;
    if ((r = mdns_listen(ctx, service_name, 1, RR_PTR, 10, stop, callback, NULL)) < 0)
        goto err;
    err:
    if (r < 0) {
        mdns_strerror(r, err, sizeof(err));
        applog_e("mDNS", "fatal: %s", err);
    }
    mdns_destroy(ctx);
    computer_discovery_running = false;
    return 0;
}