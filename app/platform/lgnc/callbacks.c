#include <stdio.h>

#include "callbacks.h"
#include "main.h"

LGNC_STATUS_T _MsgEventHandler(LGNC_MSG_TYPE_T msg, unsigned int submsg, char *pData, unsigned short dataSize)

{
    switch (msg)
    {
    case LGNC_MSG_FOCUS_IN:
        printf("LGNC_MSG_FOCUS_IN\n");
        return LGNC_OK;
    case LGNC_MSG_FOCUS_OUT:
        printf("LGNC_MSG_FOCUS_OUT\n");
        return LGNC_OK;
    case LGNC_MSG_TERMINATE:
        printf("LGNC_MSG_TERMINATE\n");
        break;
    case LGNC_MSG_HOST_EVENT:
        printf("LGNC_MSG_HOST_EVENT\n");
        break;
    case LGNC_MSG_PAUSE:
        printf("LGNC_MSG_PAUSE\n");
        return LGNC_OK;
    case LGNC_MSG_RESUME:
        printf("LGNC_MSG_RESUME\n");
        return LGNC_OK;
    }
    return LGNC_OK;
}

unsigned int _KeyEventCallback(unsigned int key, LGNC_KEY_COND_T keyCond, LGNC_ADDITIONAL_INPUT_INFO_T *keyInput)
{
    printf("KeyEvent key=%d, cond=%d\n", key, keyCond);
    return 1;
}

unsigned int _MouseEventCallback(int posX, int posY, unsigned int key, LGNC_KEY_COND_T keyCond, LGNC_ADDITIONAL_INPUT_INFO_T *keyInput)
{
    // printf("MouseEvent x=%d, y=%d key=%d, cond=%d\n", posX, posY, key, keyCond);
    if (key == 412 /* remote control back */ && keyCond == LGNC_KEY_RELEASE)
    {
        request_exit();
        return 1;
    }
    return 0;
}