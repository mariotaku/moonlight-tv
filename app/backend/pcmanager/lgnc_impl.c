#include "backend/computer_manager.h"

#include "util/bus.h"

#include <time.h>
#include <pthread.h>
#include <errno.h>

static pthread_t _timer_thread;
static pthread_mutex_t _timer_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t _timer_cond = PTHREAD_COND_INITIALIZER;
static bool _timer_running;
static void *_timer_worker(void *args);

void computer_manager_auto_discovery_start()
{
    if (_timer_running)
        return;
    _timer_running = true;
    pthread_create(&_timer_thread, NULL, _timer_worker, NULL);
}

void computer_manager_auto_discovery_stop()
{
    if (!_timer_running)
        return;
    _timer_running = false;
    pthread_cond_signal(&_timer_cond);
    pthread_join(_timer_thread, NULL);
}

void computer_manager_auto_discovery_schedule(unsigned int ms)
{
}

void *_timer_worker(void *args)
{
    while (_timer_running)
    {
        bus_pushaction((bus_actionfunc)computer_manager_run_scan, NULL);

        pthread_mutex_lock(&_timer_mutex);
        struct timespec to;
        to.tv_sec = time(NULL) + 30;
        to.tv_nsec = 0;
        int err = pthread_cond_timedwait(&_timer_cond, &_timer_mutex, &to);
        if (err == ETIMEDOUT)
        {
            pthread_mutex_unlock(&_timer_mutex);
            continue;
        }
        pthread_mutex_unlock(&_timer_mutex);
    }
    return NULL;
}