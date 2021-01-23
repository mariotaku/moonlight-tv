#include <stdlib.h>
#include <string.h>

#define BACKEND_TYPES_IMPL
#include "backend_types.h"

#define LINKEDLIST_IMPL

// Linked list functions for APP_DLIST
#define LINKEDLIST_TYPE APP_DLIST
#define LINKEDLIST_PREFIX applist
#define LINKEDLIST_DOUBLE 1
#include "util/linked_list.h"

#undef LINKEDLIST_TYPE
#undef LINKEDLIST_PREFIX
#undef LINKEDLIST_DOUBLE

// Linked list functions for SERVER_LIST
#define LINKEDLIST_TYPE SERVER_LIST
#define LINKEDLIST_PREFIX serverlist
#define LINKEDLIST_DOUBLE 1
#include "util/linked_list.h"

#undef LINKEDLIST_TYPE
#undef LINKEDLIST_PREFIX
#undef LINKEDLIST_DOUBLE
