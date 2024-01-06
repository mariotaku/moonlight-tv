#include "known_hosts.h"
#include "priv.h"
#include "pclist.h"
#include "app.h"

#include <ini.h>

#include "ini_writer.h"
#include "util/ini_ext.h"
#include "util/path.h"
#include "app_settings.h"
#include "sockaddr.h"

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

static int known_hosts_handle(known_host_t **list, const char *section, const char *name, const char *value);

static int known_hosts_find_uuid(known_host_t *node, void *v);

void pcmanager_load_known_hosts(pcmanager_t *manager) {
    commons_log_info("PCManager", "Load unknown hosts");
    char *conf_file = path_join(manager->app->settings.conf_dir, CONF_NAME_HOSTS);
    known_host_t *hosts = known_hosts_parse(conf_file);

    bool selected_set = false;
    for (known_host_t *cur = hosts; cur; cur = cur->next) {
        const char *mac = cur->mac, *hostname = cur->hostname;
        const struct sockaddr *address = cur->address;
        char address_buf[64] = {0};
        sockaddr_get_ip_str(address, address_buf, sizeof(address_buf));
        if (!mac || !hostname || !address) {
            commons_log_warn("PCManager", "Unknown host entry: mac=%s, hostname=%s, address=%s", mac, hostname,
                             address_buf);
            continue;
        }

        PSERVER_DATA server = serverdata_new();
        server->uuid = uuidstr_tostr(&cur->uuid);
        server->mac = mac;
        server->hostname = hostname;
        server->serverInfo.address = strdup(address_buf);
        server->extPort = sockaddr_get_port(address);

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
    known_hosts_free(hosts, known_hosts_node_free);
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
        char address_buf[64] = {0};
        const SERVER_DATA *server = cur->server;
        ini_write_section(fp, server->uuid);

        ini_write_string(fp, "mac", server->mac);
        ini_write_string(fp, "hostname", server->hostname);

        struct sockaddr *address = sockaddr_new();
        if (sockaddr_set_ip_str(address, strchr(server->serverInfo.address, ':') ? AF_INET6 : AF_INET,
                            server->serverInfo.address) != 0) {
            free(address);
            continue;
        }
        sockaddr_set_port(address, server->extPort);
        sockaddr_to_string(address, address_buf, sizeof(address_buf));
        ini_write_string(fp, "address", address_buf);
        free(address);

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

static int known_hosts_handle(known_host_t **list, const char *section, const char *name, const char *value) {
    if (!section) { return 1; }
    known_host_t *host = known_hosts_find_by(*list, section, (known_hosts_find_fn) known_hosts_find_uuid);
    if (!host) {
        host = known_hosts_new();
        uuidstr_fromstr(&host->uuid, section);
        *list = known_hosts_append(*list, host);
    }
    if (INI_NAME_MATCH("mac")) {
        host->mac = SDL_strdup(value);
    } else if (INI_NAME_MATCH("hostname")) {
        host->hostname = SDL_strdup(value);
    } else if (INI_NAME_MATCH("address")) {
        host->address = sockaddr_parse(value);
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

static int known_hosts_find_uuid(known_host_t *node, void *v) {
    return !uuidstr_t_equals_s(&node->uuid, v);
}

void known_hosts_node_free(known_host_t *node) {
    if (node->address) {
        free(node->address);
    }
    if (node->favs) {
        appid_list_free(node->favs, (appid_list_nodefree_fn) free);
    }
    if (node->hidden) {
        appid_list_free(node->hidden, (appid_list_nodefree_fn) free);
    }
    free(node);
}

known_host_t *known_hosts_parse(const char *conf_file) {
    known_host_t *hosts = NULL;
    if (ini_parse(conf_file, (ini_handler) known_hosts_handle, &hosts) != 0) {
        return NULL;
    }
    return hosts;
}