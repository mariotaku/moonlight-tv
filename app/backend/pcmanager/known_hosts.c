#include "priv.h"
#include "pclist.h"

#include "ini.h"

#include "util/ini_ext.h"
#include "util/path.h"
#include "stream/settings.h"

#include "pclist.h"

typedef struct known_host_t {
    char *uuid;
    char *mac;
    char *hostname;
    char *address;
    bool selected;
    struct known_host_t *next;
} known_host_t;

#define LINKEDLIST_IMPL
#define LINKEDLIST_MODIFIER static
#define LINKEDLIST_TYPE known_host_t
#define LINKEDLIST_PREFIX hostlist

#include "util/linked_list.h"

#undef LINKEDLIST_TYPE
#undef LINKEDLIST_PREFIX

static inline void strlower(char *p);

static int known_hosts_parse(known_host_t **list, const char *section, const char *name, const char *value);

static int hostlist_find_uuid(known_host_t *node, void *v);

void pcmanager_load_known_hosts(pcmanager_t *manager) {
    char *confdir = path_pref(), *conffile = path_join(confdir, CONF_NAME_HOSTS);
    free(confdir);
    known_host_t *hosts = NULL;
    if (ini_parse(conffile, (ini_handler) known_hosts_parse, &hosts) != 0) {
        goto cleanup;
    }

    bool selected_set = false;
    for (known_host_t *cur = hosts; cur; cur = cur->next) {
        const char *mac = cur->mac, *hostname = cur->hostname, *address = cur->address;
        if (!mac || !hostname || !address) {
            continue;
        }

        char *uuid = cur->uuid;
        strlower(uuid);
        PSERVER_DATA server = serverdata_new();
        server->uuid = uuid;
        server->mac = mac;
        server->hostname = hostname;
        server->serverInfo.address = address;

        SERVER_LIST *node = pclist_insert_known(manager, server);

//        config_setting_t *favs = config_setting_get_member(item, "favorites");
//        if (favs) {
//            for (int bi = 0, bj = config_setting_length(favs); bi < bj; bi++) {
//                int appid = config_setting_get_int_elem(favs, bi);
//                if (!appid) continue;
//                pcmanager_favorite_app(node, appid, true);
//            }
//        }

        if (!selected_set && cur->selected) {
            node->selected = true;
            selected_set = true;
        }
    }
    cleanup:
    hostlist_free(hosts, (hostlist_nodefree_fn *) free);
    free(conffile);
}

void pcmanager_save_known_hosts(pcmanager_t *manager) {
    char *confdir = path_pref(), *conffile = path_join(confdir, CONF_NAME_HOSTS);
    free(confdir);
    FILE *fp = fopen(conffile, "w");
    free(conffile);
    if (!fp) return;

    bool selected_set = false;
    for (PSERVER_LIST cur = manager->servers; cur != NULL; cur = cur->next) {
        if (!cur->server || !cur->known) {
            continue;
        }
        const SERVER_DATA *server = cur->server;
        ini_write_section(fp, server->uuid);

        ini_write_string(fp, "mac", server->mac);
        ini_write_string(fp, "hostname", server->hostname);
        ini_write_string(fp, "address", server->serverInfo.address);
        if (cur->favs) {
//            config_setting_t *favs = config_setting_add(item, "favorites", CONFIG_TYPE_ARRAY);
//            for (appid_list_t *idcur = cur->favs; idcur; idcur = idcur->next) {
//                config_setting_t *elem = config_setting_add(favs, NULL, CONFIG_TYPE_INT);
//                config_setting_set_int(elem, idcur->id);
//            }
        }
        if (!selected_set && cur->selected) {
            ini_write_bool(fp, "selected", true);
            selected_set = true;
        }
    }
    fclose(fp);
}

static inline void strlower(char *p) {
    for (; *p; ++p)
        *p = SDL_tolower(*p);
}

static int known_hosts_parse(known_host_t **list, const char *section, const char *name, const char *value) {
    if (!section) return 1;
    known_host_t *host = hostlist_find_by(*list, section, (hostlist_find_fn *) hostlist_find_uuid);
    if (!host) {
        host = hostlist_new();
        host->uuid = SDL_strdup(section);
        *list = hostlist_append(*list, host);
    }
    if (INI_NAME_MATCH("mac")) {
        host->mac = SDL_strdup(value);
    } else if (INI_NAME_MATCH("hostname")) {
        host->hostname = SDL_strdup(value);
    } else if (INI_NAME_MATCH("address")) {
        host->address = SDL_strdup(value);
    } else if (INI_NAME_MATCH("selected")) {
        host->selected = INI_IS_TRUE(value);
    }
    return 1;
}

static int hostlist_find_uuid(known_host_t *node, void *v) {
    return strncasecmp(node->uuid, v, 36);
}