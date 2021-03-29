#pragma once
#include "libgamestream/client.h"

typedef struct _APP_DLIST
{
  char *name;
  int id;
  struct _APP_DLIST *prev;
  struct _APP_DLIST *next;
} APP_DLIST, *PAPP_DLIST;

void applist_nodefree(PAPP_DLIST node);

#ifndef BACKEND_TYPES_IMPL
#define LINKEDLIST_TYPE APP_DLIST
#define LINKEDLIST_PREFIX applist
#define LINKEDLIST_DOUBLE 1
#include "util/linked_list.h"

#undef LINKEDLIST_TYPE
#undef LINKEDLIST_PREFIX
#undef LINKEDLIST_DOUBLE
#endif

typedef enum SERVER_STATE_ENUM
{
  SERVER_STATE_NONE,
  SERVER_STATE_ONLINE,
  SERVER_STATE_OFFLINE,
  SERVER_STATE_ERROR,
} SERVER_STATE_ENUM;

typedef union SERVER_STATE
{
  SERVER_STATE_ENUM code;
  struct
  {
    SERVER_STATE_ENUM code;
  } offline;
  struct
  {
    SERVER_STATE_ENUM code;
    int errcode;
    const char *errmsg;
  } error;
} SERVER_STATE;

typedef struct SERVER_LIST_T
{
  bool known;
  SERVER_STATE state;
  const SERVER_DATA *server;
  PAPP_DLIST apps;
  int applen;
  bool appload;
  struct SERVER_LIST_T *prev;
  struct SERVER_LIST_T *next;
} SERVER_LIST, *PSERVER_LIST;

int serverlist_compare_uuid(PSERVER_LIST other, const void *v);

#ifndef BACKEND_TYPES_IMPL
#define LINKEDLIST_TYPE SERVER_LIST
#define LINKEDLIST_PREFIX serverlist
#define LINKEDLIST_DOUBLE 1
#include "util/linked_list.h"

#undef LINKEDLIST_TYPE
#undef LINKEDLIST_PREFIX
#undef LINKEDLIST_DOUBLE
#endif
