#include "lunasynccall.h"

#include <assert.h>
#include <glib.h>

#include <unistd.h>
#include <sys/syscall.h>

static bool init_called = false;

static GCond message_cond;
static GMutex callback_mutex, message_mutex;

static LSMessage *message = NULL;

static bool _LSSyncCallback(LSHandle *sh, LSMessage *msg, void *ctx);

bool LSSyncCallInit()
{
    if (init_called)
        return true;
    g_cond_init(&message_cond);
    g_mutex_init(&callback_mutex);
    g_mutex_init(&message_mutex);
    init_called = true;
    return true;
}

LSMessage *LSWaitForMessage()
{
    g_mutex_lock(&message_mutex);
    while (!message)
        g_cond_wait(&message_cond, &message_mutex);
    LSMessage *result = message;
    message = NULL;
    g_mutex_unlock(&message_mutex);
    return result;
}

LSFilterFunc LSSyncCallbackObtain()
{
    g_mutex_lock(&callback_mutex);
    return _LSSyncCallback;
}

void LSSyncCallbackUnlock()
{
    g_mutex_unlock(&callback_mutex);
}

static bool _LSSyncCallback(LSHandle *sh, LSMessage *msg, void *ctx)
{
    LSMessageRef(msg);
    message = msg;
    g_cond_signal(&message_cond);
    g_mutex_unlock(&callback_mutex);
    return true;
}
