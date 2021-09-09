#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <sys/file.h>

#include "util/path.h"
#include "util/logging.h"

#ifndef GAMECONTROLLERDB_PLATFORM_USE
#define GAMECONTROLLERDB_PLATFORM_USE GAMECONTROLLERDB_PLATFORM
#endif

typedef struct WRITE_CONTEXT {
    CURL *curl;
    char *buf;
    size_t size;
    FILE *fp;
    int status;
} WRITE_CONTEXT;

static pthread_t update_thread;
static pthread_mutex_t update_lock = PTHREAD_MUTEX_INITIALIZER;

static void *_gamecontrollerdb_update_worker(void *);

void gamecontrollerdb_update() {
    pthread_create(&update_thread, NULL, _gamecontrollerdb_update_worker, NULL);
}

char *gamecontrollerdb_path() {
    char *confdir = path_pref(), *condb = path_join(confdir, "sdl_gamecontrollerdb.txt");
    free(confdir);
    return condb;
}

static void _write_mapping_lines(WRITE_CONTEXT *ctx) {
    if (!ctx->fp)
        return;
    char *curLine = ctx->buf;
    while (curLine) {
        char *nextLine = memchr(curLine, '\n', ctx->size - (curLine - ctx->buf));
        if (nextLine) {
            *nextLine = '\0';
            const char *plat_substr = "platform:" GAMECONTROLLERDB_PLATFORM_USE ",";
            const size_t plat_substr_len = sizeof("platform:" GAMECONTROLLERDB_PLATFORM_USE ",") - 1;
            char *plat = strstr(curLine, plat_substr);
            if (plat) {
                char *platEnd = plat + plat_substr_len;
                if (platEnd < nextLine) {
                    size_t lineLen = nextLine - curLine;
                    memmove(plat, platEnd, nextLine - platEnd);
                    *(plat + (nextLine - platEnd)) = '\0';
                } else {
                    *plat = '\0';
                }
                fprintf(ctx->fp, "%splatform:%s,\n", curLine, GAMECONTROLLERDB_PLATFORM);
            }
            curLine = nextLine + 1;
        } else {
            size_t rem = ctx->size - (curLine - ctx->buf);
            ctx->size = rem;
            memmove(ctx->buf, curLine, rem);
            break;
        }
    }
}

static size_t _body_cb(void *buffer, size_t size, size_t nitems, WRITE_CONTEXT *ctx) {
    if (ctx->status < 0)
        return 0;
    size_t realsize = size * nitems;
    ctx->buf = realloc(ctx->buf, ctx->size + realsize + 1);
    if (ctx->buf == NULL)
        return 0;

    memcpy(&(ctx->buf[ctx->size]), buffer, realsize);
    ctx->size += realsize;
    ctx->buf[ctx->size] = 0;
    _write_mapping_lines(ctx);
    return realsize;
}

static void _write_header_lines(WRITE_CONTEXT *ctx) {
    if (!ctx->fp)
        return;
    char *curLine = ctx->buf;
    while (curLine) {
        char *nextLine = memchr(curLine, '\n', ctx->size - (curLine - ctx->buf));
        if (nextLine) {
            *nextLine = '\0';
            if (nextLine > curLine)
                nextLine[-1] = '\0';
            char *headerValue = strstr(curLine, ": ");
            if (headerValue && strncasecmp(curLine, "etag", headerValue - curLine) == 0) {
                fprintf(ctx->fp, "# etag: %s\n", headerValue + 2);
            }
            curLine = nextLine + 1;
        } else {
            size_t rem = ctx->size - (curLine - ctx->buf);
            ctx->size = rem;
            memmove(ctx->buf, curLine, rem);
            break;
        }
    }
}

static size_t _header_cb(char *buffer, size_t size, size_t nitems, WRITE_CONTEXT *ctx) {
    if (ctx->status < 0)
        return 0;
    /* received header is nitems * size long in 'buffer' NOT ZERO TERMINATED */
    /* 'userdata' is set with CURLOPT_HEADERDATA */
    size_t realsize = size * nitems;
    ctx->buf = realloc(ctx->buf, ctx->size + realsize + 1);
    if (ctx->buf == NULL)
        return 0;

    memcpy(&(ctx->buf[ctx->size]), buffer, realsize);
    ctx->size += realsize;
    ctx->buf[ctx->size] = 0;
    if (!ctx->status || ctx->status == 301 || ctx->status == 302) {
        curl_easy_getinfo(ctx->curl, CURLINFO_RESPONSE_CODE, &ctx->status);
        if (!ctx->fp && ctx->status == 200) {
            char *condb = gamecontrollerdb_path();
            ctx->fp = fopen(condb, "w");
            applog_d("GameControllerDB", "Locking controller db file %s", condb);
            free(condb);
#ifndef __WIN32
            if (lockf(fileno(ctx->fp), F_LOCK, 0) != 0)
            {
                ctx->status = -1;
            }
#endif
        }
    }
    _write_header_lines(ctx);
    return realsize;
}

static void load_headers(struct curl_slist **headers) {
    char *condb = gamecontrollerdb_path();
    FILE *fp = fopen(condb, "r");
    free(condb);
    if (!fp)
        return;

    char linebuf[1024];
    size_t linelen;
    while (fgets(linebuf, 1024, fp)) {
        if (linebuf[0] != '#')
            break;
        linebuf[strnlen(linebuf, 1024) - 1] = '\0';
        if (strncmp(linebuf, "# etag: ", 8) == 0) {
            char headbuf[2048];
            snprintf(headbuf, 2047, "if-none-match: %s", &linebuf[8]);
            *headers = curl_slist_append(*headers, headbuf);
        }
    }

    fclose(fp);
}

void *_gamecontrollerdb_update_worker(void *unused) {
    pthread_mutex_lock(&update_lock);
    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL,
                     "https://github.com/gabomdq/SDL_GameControllerDB/raw/master/gamecontrollerdb.txt");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, _header_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _body_cb);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
    struct curl_slist *headers = NULL;
    load_headers(&headers);
    if (headers) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }
    WRITE_CONTEXT *ctx = malloc(sizeof(WRITE_CONTEXT));
    ctx->curl = curl;
    ctx->buf = malloc(1);
    ctx->size = 0;
    ctx->fp = NULL;
    ctx->status = 0;

    curl_easy_setopt(curl, CURLOPT_WRITEDATA, ctx);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, ctx);
    CURLcode res = curl_easy_perform(curl);
    if (ctx->status == 304) {
        applog_d("GameControllerDB", "Controller db has no update");
    }
    if (ctx->fp) {
        applog_d("GameControllerDB", "Unlocking controller db file");
#ifndef __WIN32
        lockf(fileno(ctx->fp), F_ULOCK, 0);
#endif
        fclose(ctx->fp);
    }
    free(ctx->buf);
    free(ctx);
    if (headers) {
        curl_slist_free_all(headers);
    }
    curl_easy_cleanup(curl);
    pthread_mutex_unlock(&update_lock);
    return NULL;
}