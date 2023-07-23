#include "backend/pcmanager.h"
#include "backend/pcmanager/priv.h"
#include "backend/pcmanager/worker/worker.h"

#include <dns_sd.h>
#include <SDL_thread.h>
#include <sys/poll.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>

#include "logging.h"

struct discovery_task_t {
    pcmanager_t *manager;
    SDL_mutex *lock;
    SDL_Thread *thread;
    bool stop;
};

static int discovery_worker(discovery_task_t *task);

static bool discovery_stopped(discovery_task_t *task);

static void browse_callback(
        DNSServiceRef sdRef,
        DNSServiceFlags flags,
        uint32_t interfaceIndex,
        DNSServiceErrorType errorCode,
        const char *serviceName,
        const char *regtype,
        const char *replyDomain,
        void *context
);

static void resolve_callback(
        DNSServiceRef sdRef,
        DNSServiceFlags flags,
        uint32_t interfaceIndex,
        DNSServiceErrorType errorCode,
        const char *fullname,
        const char *hosttarget,
        uint16_t port,                  /* In network byte order */
        uint16_t txtLen,
        const unsigned char *txtRecord,
        void *context
);

static void discovery_finalize(void *arg, int result);

void pcmanager_auto_discovery_start(pcmanager_t *manager) {
    pcmanager_lock(manager);
    if (manager->discovery_task != NULL) {
        pcmanager_unlock(manager);
        return;
    }
    discovery_task_t *task = SDL_calloc(1, sizeof(discovery_task_t));
    task->manager = manager;
    task->lock = SDL_CreateMutex();
    task->stop = false;
    task->thread = SDL_CreateThread((SDL_ThreadFunction) discovery_worker, "discovery", task);
    manager->discovery_task = task;
    pcmanager_unlock(manager);
}

void pcmanager_auto_discovery_stop(pcmanager_t *manager) {
    pcmanager_lock(manager);
    discovery_task_t *task = manager->discovery_task;
    if (task == NULL) {
        pcmanager_unlock(manager);
        return;
    }
    manager->discovery_task = NULL;
    executor_submit(manager->executor, executor_noop, discovery_finalize, task);
    pcmanager_unlock(manager);
}

static void browse_callback(
        DNSServiceRef sdRef,
        DNSServiceFlags flags,
        uint32_t interfaceIndex,
        DNSServiceErrorType errorCode,
        const char *serviceName,
        const char *regtype,
        const char *replyDomain,
        void *context
) {
    (void) sdRef;
    (void) flags;
    if (errorCode != kDNSServiceErr_NoError) {
        commons_log_warn("Discovery", "Got error: %d", errorCode);
        return;
    }
    DNSServiceRef dns;
    if (DNSServiceResolve(&dns, 0, interfaceIndex, serviceName, regtype, replyDomain, resolve_callback, context) !=
        kDNSServiceErr_NoError) {
        commons_log_warn("Discovery", "DNSServiceResolve failed");
        return;
    }
    DNSServiceProcessResult(dns);
    DNSServiceRefDeallocate(dns);
}

static void resolve_callback(
        DNSServiceRef sdRef,
        DNSServiceFlags flags,
        uint32_t interfaceIndex,
        DNSServiceErrorType errorCode,
        const char *fullname,
        const char *hosttarget,
        uint16_t port,                  /* In network byte order */
        uint16_t txtLen,
        const unsigned char *txtRecord,
        void *context
) {
    (void) sdRef;
    (void) flags;
    if (errorCode != kDNSServiceErr_NoError) {
        commons_log_warn("Discovery", "DNSServiceResolve got error: %d", errorCode);
        return;
    }
    struct hostent *host = gethostbyname2(hosttarget, AF_INET);
    if (host == NULL) {
        return;
    }
    if (host->h_length <= 0) {
        return;
    }
    char buf[128];
    const char *ip = inet_ntop(AF_INET, host->h_addr_list[0], buf, sizeof(buf));

    discovery_task_t *task = context;
    worker_context_t *ctx = worker_context_new(task->manager, NULL, NULL, NULL);
    ctx->arg1 = strdup(ip);
    pcmanager_worker_queue(task->manager, worker_host_discovered, ctx);
}

int discovery_worker(discovery_task_t *task) {
    while (!discovery_stopped(task)) {
        DNSServiceRef dns;
        if (DNSServiceBrowse(&dns, 0, 0, "_nvstream._tcp", "", browse_callback, task) != kDNSServiceErr_NoError) {
            commons_log_warn("Discovery", "DNSServiceBrowse failed");
            return -1;
        }
        int mdnsfd = DNSServiceRefSockFD(dns);
        struct pollfd fds = {
                .fd = mdnsfd,
                .events = POLLIN | POLLHUP
        };
        int ret = poll(&fds, 1, 3000);
        if (ret <= 0 && errno != EINTR) {
            DNSServiceRefDeallocate(dns);
            continue;
        }

        if (fds.revents & (POLLIN | POLLHUP | POLLERR)) {
            /* Invoke callback function */
            if (DNSServiceProcessResult(dns) != kDNSServiceErr_NoError) {
                DNSServiceRefDeallocate(dns);
                continue;
            }
        }
        DNSServiceRefDeallocate(dns);
        sleep(5);
    }
    return 0;
}

static bool discovery_stopped(discovery_task_t *task) {
    SDL_LockMutex(task->lock);
    bool stop = task->stop;
    SDL_UnlockMutex(task->lock);
    return stop;
}

static void discovery_finalize(void *arg, int result) {
    (void) result;
    discovery_task_t *task = arg;
    SDL_LockMutex(task->lock);
    task->stop = true;
    SDL_UnlockMutex(task->lock);
    SDL_WaitThread(task->thread, NULL);
    SDL_DestroyMutex(task->lock);
    free(task);
}
