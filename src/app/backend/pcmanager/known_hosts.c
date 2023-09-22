#include "priv.h"
#include "pclist.h"
#include "app.h"

#include <ini.h>

#include "ini_writer.h"
#include "util/ini_ext.h"
#include "util/path.h"
#include "app_settings.h"

typedef struct known_host_t {
    uuidstr_t uuid;
    char *mac;
    char *hostname;
    char *address;
    bool selected;
    appid_list_t *favs;
    appid_list_t *hidden;
    struct known_host_t *next;
} known_host_t;

#define LINKEDLIST_IMPL
#define LINKEDLIST_MODIFIER static
#define LINKEDLIST_TYPE known_host_t
#define LINKEDLIST_PREFIX hostlist

#include "linked_list.h"

#undef LINKEDLIST_TYPE
#undef LINKEDLIST_PREFIX

#define LINKEDLIST_IMPL
#define LINKEDLIST_MODIFIER static
#define LINKEDLIST_TYPE appid_list_t
#define LINKEDLIST_PREFIX appid_list
#define LINKEDLIST_DOUBLE 1

#include "linked_list.h"
#include "logging.h"

#undef LINKEDLIST_DOUBLE
#undef LINKEDLIST_TYPE
#undef LINKEDLIST_PREFIX

static int known_hosts_parse(known_host_t **list, const char *section, const char *name, const char *value);

static int hostlist_find_uuid(known_host_t *node, void *v);

static void hostlist_node_free(known_host_t *node);

void pcmanager_load_known_hosts(pcmanager_t *manager) {
    commons_log_info("PCManager", "Load unknown hosts");
    char *conf_file = path_join(manager->app->settings.conf_dir, CONF_NAME_HOSTS);
    known_host_t *hosts = NULL;
    if (ini_parse(conf_file, (ini_handler) known_hosts_parse, &hosts) != 0) {
        goto cleanup;
    }

    bool selected_set = false;
    for (known_host_t *cur = hosts; cur; cur = cur->next) {
        const char *mac = cur->mac, *hostname = cur->hostname, *address = cur->address;
        if (!mac || !hostname || !address) {
            commons_log_warn("PCManager", "Unknown host entry: mac=%s, hostname=%s, address=%s", mac, hostname,
                             address);
            continue;
        }

        PSERVER_DATA server = serverdata_new();
        server->uuid = uuidstr_tostr(&cur->uuid);
        server->mac = mac;
        server->hostname = hostname;
        server->serverInfo.address = address;

        pclist_t *node = pclist_insert_known(manager, &cur->uuid, server);

        node->favs = cur->favs;
        cur->favs = NULL;
        node->hidden = cur->hidden;
        cur->hidden = NULL;

        if (!selected_set && cur->selected) {
            commons_log_info("PCManager", "Known host %s was selected", hosts->hostname);
            node->selected = true;
            selected_set = true;
        }
    }
    cleanup:
    hostlist_free(hosts, hostlist_node_free);
    free(conf_file);
}

void pcmanager_save_known_hosts(pcmanager_t *manager) {
    char *conf_file = path_join(manager->app->settings.conf_dir, CONF_NAME_HOSTS);
    FILE *fp = fopen(conf_file, "wb");
    free(conf_file);
    if (!fp) { return; }

    bool selected_set = false;
    for (pclist_t *cur = manager->servers; cur != NULL; cur = cur->next) {
        if (!cur->server || !cur->known) {
            continue;
        }
        const SERVER_DATA *server = cur->server;
        ini_write_section(fp, server->uuid);

        ini_write_string(fp, "mac", server->mac);
        ini_write_string(fp, "hostname", server->hostname);
        ini_write_string(fp, "address", server->serverInfo.address);
        if (!selected_set && cur->selected) {
            ini_write_bool(fp, "selected", true);
            selected_set = true;
        }
        if (cur->favs) {
            ini_write_comment(fp, " favorites list");
            for (appid_list_t *idcur = cur->favs; idcur; idcur = idcur->next) {
                ini_write_int(fp, "favorite", idcur->id);
            }
        }
        if (cur->hidden) {
            ini_write_comment(fp, " hidden apps list");
            for (appid_list_t *idcur = cur->hidden; idcur; idcur = idcur->next) {
                ini_write_int(fp, "hidden", idcur->id);
            }
        }
    }
    fclose(fp);
}

static int known_hosts_parse(known_host_t **list, const char *section, const char *name, const char *value) {
    if (!section) { return 1; }
    known_host_t *host = hostlist_find_by(*list, section, (hostlist_find_fn) hostlist_find_uuid);
    if (!host) {
        host = hostlist_new();
        uuidstr_fromstr(&host->uuid, section);
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
    } else if (INI_NAME_MATCH("favorite")) {
        appid_list_t *id_item = appid_list_new();
        id_item->id = SDL_atoi(value);
        host->favs = appid_list_append(host->favs, id_item);
    } else if (INI_NAME_MATCH("hidden")) {
        appid_list_t *id_item = appid_list_new();
        id_item->id = SDL_atoi(value);
        host->hidden = appid_list_append(host->hidden, id_item);
    }
    return 1;
}

static int hostlist_find_uuid(known_host_t *node, void *v) {
    return !uuidstr_t_equals_s(&node->uuid, v);
}

static void hostlist_node_free(known_host_t *node) {
    if (node->favs) {
        appid_list_free(node->favs, (appid_list_nodefree_fn) free);
    }
    if (node->hidden) {
        appid_list_free(node->hidden, (appid_list_nodefree_fn) free);
    }
    free(node);
}