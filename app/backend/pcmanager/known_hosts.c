#include "priv.h"
#include "pclist.h"

#include <libconfig.h>

#include "util/libconfig_ext.h"
#include "util/path.h"
#include "stream/settings.h"


static void strlower(char *p);

void pcmanager_load_known_hosts(pcmanager_t *manager) {
    char *confdir = path_pref(), *conffile = path_join(confdir, CONF_NAME_HOSTS);
    free(confdir);
    config_t config;
    config_init(&config);
    int options = config_get_options(&config);
    options &= ~CONFIG_OPTION_OPEN_BRACE_ON_SEPARATE_LINE;
    options &= ~CONFIG_OPTION_COLON_ASSIGNMENT_FOR_GROUPS;
    config_set_options(&config, options);
    if (config_read_file(&config, conffile) != CONFIG_TRUE) {
        goto cleanup;
    }
    config_setting_t *root = config_root_setting(&config);
    bool selected_set = false;
    for (int i = 0, j = config_setting_length(root); i < j; i++) {
        config_setting_t *item = config_setting_get_elem(root, i);
        const char *mac = config_setting_get_string_simple(item, "mac"),
                *hostname = config_setting_get_string_simple(item, "hostname"),
                *address = config_setting_get_string_simple(item, "address");
        if (!mac || !hostname || !address) {
            continue;
        }
        const char *key = config_setting_name(item);
        int keyoff = key[0] == '*' ? 1 : 0;
        char *uuid = SDL_strdup(&key[keyoff]);
        PSERVER_DATA server = serverdata_new();
        server->uuid = uuid;
        server->mac = SDL_strdup(mac);
        server->hostname = SDL_strdup(hostname);
        server->serverInfo.address = SDL_strdup(address);

        SERVER_LIST *node = pclist_insert_known(manager, server);
        if (!selected_set && config_setting_get_bool_simple(item, "selected")) {
            node->selected = true;
            selected_set = true;
        }
    }
    cleanup:
    config_destroy(&config);
    free(conffile);
}

void pcmanager_save_known_hosts(pcmanager_t *manager) {
    config_t config;
    config_init(&config);
    int options = config_get_options(&config);
    options &= ~CONFIG_OPTION_OPEN_BRACE_ON_SEPARATE_LINE;
    options &= ~CONFIG_OPTION_COLON_ASSIGNMENT_FOR_GROUPS;
    config_set_options(&config, options);
    config_setting_t *root = config_root_setting(&config);
    bool selected_set = false;
    for (PSERVER_LIST cur = manager->servers; cur != NULL; cur = cur->next) {
        if (!cur->server || !cur->known) {
            continue;
        }
        const SERVER_DATA *server = cur->server;
        char key[38];
        key[0] = '*';
        SDL_memcpy(&key[1], server->uuid, 36);
        key[37] = '\0';
        strlower(&key[1]);
        config_setting_t *item = config_setting_add(root, key, CONFIG_TYPE_GROUP);
        if (!item) {
            continue;
        }
        config_setting_set_string_simple(item, "mac", server->mac);
        config_setting_set_string_simple(item, "hostname", server->hostname);
        config_setting_set_string_simple(item, "address", server->serverInfo.address);
        if (!selected_set && cur->selected) {
            config_setting_set_bool_simple(item, "selected", true);
            selected_set = true;
        }
    }
    char *confdir = path_pref(), *conffile = path_join(confdir, CONF_NAME_HOSTS);
    free(confdir);
    config_write_file(&config, conffile);
    cleanup:
    config_destroy(&config);
    free(conffile);
}

static inline void strlower(char *p) {
    for (; *p; ++p)
        *p = SDL_tolower(*p);
}