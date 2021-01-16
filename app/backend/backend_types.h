#pragma once
#include "libgamestream/client.h"

typedef struct _APP_DLIST {
  char* name;
  int id;
  struct _APP_DLIST *prev;
  struct _APP_DLIST *next;
} APP_DLIST, *PAPP_DLIST;

typedef struct SERVER_LIST_T
{
    char *address;
    char *name;
    PSERVER_DATA server;
    int err;
    const char *errmsg;
    PAPP_DLIST apps;
    struct SERVER_LIST_T *next;
} SERVER_LIST, *PSERVER_LIST;
