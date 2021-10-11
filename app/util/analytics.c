//
// Created by Mariotaku on 2021/10/11.
//

#include "analytics.h"
#include "path.h"
#include "os_info.h"
#include <SDL.h>
#include <curl/curl.h>
#include <uuid/uuid.h>
#include "app.h"

struct analytics_event {
    char *dp;
};

static int event_send(void *arg);

static void event_serialize(CURL *curl, struct analytics_event *event, char *out, size_t limit);

static void init_cid();

static uuid_t cid;
static os_info_t os_info;

void analytics_start() {
    if (!app_configuration->analytics) return;
    init_cid();
    os_info_get(&os_info);
    SDL_CreateThread(event_send, "analytics", NULL);
}

static void init_cid() {
    memset(cid, 0, sizeof(cid));
    char *confdir = path_pref(), *conffile = path_join(confdir, "analytics_cid.bin");
    free(confdir);
    FILE *f = fopen(conffile, "rb");
    if (f) {
        if (fread(cid, 1, sizeof(cid), f) == sizeof(cid)) {
            goto cleanup;
        } else {
            fclose(f);
        }
    }
    f = fopen(conffile, "wb");
    if (!f) goto cleanup;
    uuid_generate_random(cid);
    fwrite(cid, sizeof(cid), 1, f);
    fflush(f);
    cleanup:
    free(conffile);
    fclose(f);
}

static int event_send(void *arg) {
    CURL * curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, "https://www.google-analytics.com/collect");
    struct curl_slist *headers = NULL;
    char user_agent[512];
    snprintf(user_agent, sizeof(user_agent), "User-Agent: Moonlight-TV/%s (%s/%s)", APP_VERSION, os_info.name,
             os_info.release);
    headers = curl_slist_append(headers, user_agent);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    struct analytics_event event = {.dp = "/"};
    char data[8192];
    event_serialize(curl, &event, data, sizeof(data));
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
    CURLcode ret = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return ret;
}

static void event_serialize(CURL *curl, struct analytics_event *event, char *out, size_t limit) {
    char cid_str[UUID_STR_LEN];
    uuid_unparse(cid, cid_str);
    char *dp_param = curl_easy_escape(curl, event->dp, strlen(event->dp));
    snprintf(out, limit, "v=1&tid=UA-209928110-1&cid=%s&t=pageview&dp=%s", cid_str, dp_param);
    curl_free(dp_param);
}

