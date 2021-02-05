#pragma once
#include "libgamestream/client.h"

typedef struct _APP_DLIST
{
  char *name;
  int id;
  struct _APP_DLIST *prev;
  struct _APP_DLIST *next;
} APP_DLIST, *PAPP_DLIST;

#ifndef BACKEND_TYPES_IMPL
#define LINKEDLIST_TYPE APP_DLIST
#define LINKEDLIST_PREFIX applist
#define LINKEDLIST_DOUBLE 1
#include "util/linked_list.h"

#undef LINKEDLIST_TYPE
#undef LINKEDLIST_PREFIX
#undef LINKEDLIST_DOUBLE
#endif

typedef struct SERVER_LIST_T
{
  bool known;
  char *uuid;
  char *mac;
  char *hostname;
  PSERVER_DATA server;
  int err;
  const char *errmsg;
  PAPP_DLIST apps;
  int applen;
  bool appload;
  struct SERVER_LIST_T *prev;
  struct SERVER_LIST_T *next;
} SERVER_LIST, *PSERVER_LIST;

#ifndef BACKEND_TYPES_IMPL
#define LINKEDLIST_TYPE SERVER_LIST
#define LINKEDLIST_PREFIX serverlist
#define LINKEDLIST_DOUBLE 1
#include "util/linked_list.h"

#undef LINKEDLIST_TYPE
#undef LINKEDLIST_PREFIX
#undef LINKEDLIST_DOUBLE
#endif
