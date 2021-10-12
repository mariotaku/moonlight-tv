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

#ifndef UUID_STR_LEN
#define UUID_STR_LEN 37
#endif

typedef enum analytics_event_type {
    ANALYTICS_TYPE_PAGEVIEW,
    ANALYTICS_TYPE_EVENT,
} analytics_event_type;

typedef union analytics_event {
    analytics_event_type type;
    struct {
        analytics_event_type type;
        char dp[64];
    } pageview;
    struct {
        analytics_event_type type;
        char ec[32];
        char ea[32];
        bool ni;
    } event;
} analytics_event;

static int event_send(analytics_event *event);

static void event_serialize(CURL *curl, analytics_event *event, char *out, size_t limit);

static void init_cid();

static Uint32 analytics_heartbeat(Uint32 interval, void *arg);

static uuid_t cid;
static os_info_t os_info;

void analytics_start() {
    if (!app_configuration->analytics) return;
    init_cid();
    os_info_get(&os_info);
    analytics_event *event = SDL_malloc(sizeof(analytics_event));
    SDL_memset(event, 0, sizeof(analytics_event));
    event->type = ANALYTICS_TYPE_PAGEVIEW;
    SDL_strlcpy(event->pageview.dp, "/", sizeof(event->pageview.dp));
    SDL_CreateThread((SDL_ThreadFunction) event_send, "analytics", event);
    SDL_AddTimer(27 * 60 * 1000, analytics_heartbeat, NULL);
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

static int event_send(analytics_event *event) {
    CURL * curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, "https://www.google-analytics.com/collect");
    struct curl_slist *headers = NULL;
    char user_agent[512];
    SDL_snprintf(user_agent, sizeof(user_agent), "User-Agent: Moonlight-TV/%s (%s/%s)", APP_VERSION,
                 os_info.name, os_info.release);
    headers = curl_slist_append(headers, user_agent);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    char data[8192];
    event_serialize(curl, event, data, sizeof(data));
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
    CURLcode ret = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    free(event);
    return ret;
}

static void event_serialize(CURL *curl, analytics_event *event, char *out, size_t limit) {
    char cid_str[UUID_STR_LEN];
#if __WIN32

#else
    uuid_unparse(cid, cid_str);
#endif
    switch (event->type) {
        case ANALYTICS_TYPE_PAGEVIEW: {
            char *dp_param = curl_easy_escape(curl, event->pageview.dp, SDL_strlen(event->pageview.dp));
            SDL_snprintf(out, limit, "v=1&tid=UA-209928110-1&cid=%s&t=pageview&dp=%s", cid_str, dp_param);
            curl_free(dp_param);
            break;
        }
        case ANALYTICS_TYPE_EVENT: {
            char *ec_param = curl_easy_escape(curl, event->event.ec, SDL_strlen(event->event.ec));
            char *ea_param = curl_easy_escape(curl, event->event.ea, SDL_strlen(event->event.ea));
            SDL_snprintf(out, limit, "v=1&tid=UA-209928110-1&cid=%s&t=event&ec=%s&ea=%s%s", cid_str, ec_param, ea_param,
                         event->event.ni ? "&ni=1" : "");
            curl_free(ea_param);
            break;
        }
    }
}

static Uint32 analytics_heartbeat(Uint32 interval, void *arg) {
    analytics_event *event = SDL_malloc(sizeof(analytics_event));
    SDL_memset(event, 0, sizeof(analytics_event));
    event->type = ANALYTICS_TYPE_EVENT;
    event->event.ni = true;
    SDL_strlcpy(event->event.ec, "session", sizeof(event->event.ec));
    SDL_strlcpy(event->event.ea, "heartbeat", sizeof(event->event.ea));
    SDL_CreateThread((SDL_ThreadFunction) event_send, "analytics", event);
    return interval;
}