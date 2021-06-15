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

typedef struct WRITE_CONTEXT
{
    char *buf;
    size_t size;
    FILE *fp;
} WRITE_CONTEXT;

static pthread_t update_thread;
static pthread_mutex_t update_lock = PTHREAD_MUTEX_INITIALIZER;

static void *_gamecontrollerdb_update_worker(void *);

void gamecontrollerdb_update()
{
    pthread_create(&update_thread, NULL, _gamecontrollerdb_update_worker, NULL);
}

char *gamecontrollerdb_path()
{
    char *confdir = path_pref(), *condb = path_join(confdir, "sdl_gamecontrollerdb.txt");
    free(confdir);
    return condb;
}

static void _write_mapping(WRITE_CONTEXT *ctx)
{
    char *curLine = ctx->buf;
    while (curLine)
    {
        char *nextLine = memchr(curLine, '\n', ctx->size - (curLine - ctx->buf));
        if (nextLine)
        {
            *nextLine = '\0';
            const char *plat_substr = "platform:" GAMECONTROLLERDB_PLATFORM_USE ",";
            const size_t plat_substr_len = sizeof("platform:" GAMECONTROLLERDB_PLATFORM_USE ",") - 1;
            char *plat = strstr(curLine, plat_substr);
            if (plat)
            {
                char *platEnd = plat + plat_substr_len;
                if (platEnd < nextLine)
                {
                    size_t lineLen = nextLine - curLine;
                    memmove(plat, platEnd, nextLine - platEnd);
                    *(plat + (nextLine - platEnd)) = '\0';
                }
                else
                {
                    *plat = '\0';
                }
                fprintf(ctx->fp, "%splatform:%s,\n", curLine, GAMECONTROLLERDB_PLATFORM);
            }
            curLine = nextLine + 1;
        }
        else
        {
            size_t rem = ctx->size - (curLine - ctx->buf);
            ctx->size = rem;
            memmove(ctx->buf, curLine, rem);
            break;
        }
    }
}

static size_t _mappings_writef(void *contents, size_t size, size_t nmemb, WRITE_CONTEXT *ctx)
{
    size_t realsize = size * nmemb;
    ctx->buf = realloc(ctx->buf, ctx->size + realsize + 1);
    if (ctx->buf == NULL)
        return 0;

    memcpy(&(ctx->buf[ctx->size]), contents, realsize);
    ctx->size += realsize;
    ctx->buf[ctx->size] = 0;
    _write_mapping(ctx);
    return realsize;
}

void *_gamecontrollerdb_update_worker(void *unused)
{
    pthread_mutex_lock(&update_lock);
    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, "https://github.com/gabomdq/SDL_GameControllerDB/raw/master/gamecontrollerdb.txt");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _mappings_writef);
    WRITE_CONTEXT *ctx = malloc(sizeof(WRITE_CONTEXT));
    ctx->buf = malloc(1);
    ctx->size = 0;

    char *condb = gamecontrollerdb_path();
    ctx->fp = fopen(condb, "w");
    applog_d("GameControllerDB", "Locking controller db file %s", condb);
    free(condb);
    if (lockf(fileno(ctx->fp), F_LOCK, 0) != 0)
    {
        goto cleanup;
    }
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, ctx);
    CURLcode res = curl_easy_perform(curl);
    applog_d("GameControllerDB", "Unlocking controller db file");
    lockf(fileno(ctx->fp), F_ULOCK, 0);
cleanup:
    fclose(ctx->fp);
    free(ctx->buf);
    free(ctx);
    curl_easy_cleanup(curl);
    pthread_mutex_unlock(&update_lock);
    return NULL;
}