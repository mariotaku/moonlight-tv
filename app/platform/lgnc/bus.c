#include <message_queue.h>

#include "util/bus.h"

struct message_queue queue;

struct MESSAGE_T
{
    int which;
    void *data1;
    void *data2;
};

void bus_init()
{
    message_queue_init(&queue, sizeof(struct MESSAGE_T), 256);
}

void bus_destroy()
{
    message_queue_destroy(&queue);
}

bool bus_pushevent(int which, void *data1, void *data2)
{
    struct MESSAGE_T *msg = message_queue_message_alloc_blocking(&queue);
    if (!msg)
    {
        return false;
    }
    msg->which = which;
    msg->data1 = data1;
    msg->data2 = data2;
    message_queue_write(&queue, msg);
    return true;
}

bool bus_pollevent(int *which, void **data1, void **data2)
{
    struct MESSAGE_T *msg = message_queue_tryread(&queue);
    if (!msg)
    {
        return false;
    }
    *which = msg->which;
    *data1 = msg->data1;
    *data2 = msg->data2;
    message_queue_message_free(&queue, msg);
    return true;
}