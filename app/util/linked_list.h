#include <string.h>
#include <stdlib.h>
#include "util/memlog.h"

#ifndef LINKEDLIST_TYPE
#error "Please define LINKEDLIST_TYPE before include"
#endif
#ifndef LINKEDLIST_PREFIX
#define LINKEDLIST_PREFIX linkedlist
#endif

// Coming from https://stackoverflow.com/a/1489985/859190
#define LINKEDLIST_DECL_PASTER(x, y) x##_##y
#define LINKEDLIST_DECL_EVALUATOR(x, y) LINKEDLIST_DECL_PASTER(x, y)
#define LINKEDLIST_FN_NAME(name) LINKEDLIST_DECL_EVALUATOR(LINKEDLIST_PREFIX, name)

typedef int(LINKEDLIST_FN_NAME(find_fn))(LINKEDLIST_TYPE *p, const void *fv);
typedef int(LINKEDLIST_FN_NAME(compare_fn))(LINKEDLIST_TYPE *p1, LINKEDLIST_TYPE *p2);
typedef void(LINKEDLIST_FN_NAME(nodefree_fn))(LINKEDLIST_TYPE *p);

#ifndef LINKEDLIST_IMPL
LINKEDLIST_TYPE *LINKEDLIST_FN_NAME(new)();
int LINKEDLIST_FN_NAME(len)(LINKEDLIST_TYPE *p);
LINKEDLIST_TYPE *LINKEDLIST_FN_NAME(nth)(LINKEDLIST_TYPE *p, int n);
LINKEDLIST_TYPE *LINKEDLIST_FN_NAME(tail)(LINKEDLIST_TYPE *p);
int LINKEDLIST_FN_NAME(index)(LINKEDLIST_TYPE *p, LINKEDLIST_TYPE *f);
LINKEDLIST_TYPE *LINKEDLIST_FN_NAME(find_by)(LINKEDLIST_TYPE *p, const void *v, LINKEDLIST_FN_NAME(find_fn) fn);
LINKEDLIST_TYPE *LINKEDLIST_FN_NAME(append)(LINKEDLIST_TYPE *p, LINKEDLIST_TYPE *node);
#if LINKEDLIST_DOUBLE
LINKEDLIST_TYPE *LINKEDLIST_FN_NAME(sortedinsert)(LINKEDLIST_TYPE *p, LINKEDLIST_TYPE *node, LINKEDLIST_FN_NAME(compare_fn) fn);
#endif
void LINKEDLIST_FN_NAME(free)(LINKEDLIST_TYPE *head, LINKEDLIST_FN_NAME(nodefree_fn) fn);
#else
LINKEDLIST_TYPE *LINKEDLIST_FN_NAME(new)()
{
    LINKEDLIST_TYPE *node = malloc(sizeof(LINKEDLIST_TYPE));
    memset(node, 0, sizeof(LINKEDLIST_TYPE));
#if LINKEDLIST_DOUBLE
    node->prev = NULL;
#endif
    node->next = NULL;
    return node;
}

int LINKEDLIST_FN_NAME(len)(LINKEDLIST_TYPE *p)
{
    int length = 0;
    for (LINKEDLIST_TYPE *cur = p; cur != NULL; cur = cur->next)
    {
        length++;
    }
    return length;
}

LINKEDLIST_TYPE *LINKEDLIST_FN_NAME(nth)(LINKEDLIST_TYPE *p, int n)
{
    LINKEDLIST_TYPE *ret = NULL;
    int i = 0;
#if LINKEDLIST_DOUBLE
    if (n < 0)
    {
        for (ret = p; ret != NULL && i > n; ret = ret->prev, i--)
            ;
    }
    else
#endif
    {
        for (ret = p; ret != NULL && i < n; ret = ret->next, i++)
            ;
    }
    return ret;
}

LINKEDLIST_TYPE *LINKEDLIST_FN_NAME(top)(LINKEDLIST_TYPE *p)
{
    if (!p)
        return NULL;
    LINKEDLIST_TYPE *cur = p;
    while (cur->next != NULL)
    {
        cur = cur->next;
    }
    return cur;
}

int LINKEDLIST_FN_NAME(index)(LINKEDLIST_TYPE *p, LINKEDLIST_TYPE *f)
{
    int i = 0;

    LINKEDLIST_TYPE *cur;
    for (cur = p; cur != NULL; cur = cur->next, i++)
    {
        if (cur == f)
        {
            return i;
        }
    }
    return -1;
}

LINKEDLIST_TYPE *LINKEDLIST_FN_NAME(find_by)(LINKEDLIST_TYPE *p, const void *v, LINKEDLIST_FN_NAME(find_fn) fn)
{
    LINKEDLIST_TYPE *ret = NULL;
    int i = 0;
    for (ret = p; ret != NULL && fn(ret, v) != 0; ret = ret->next, i++)
        ;
    return ret;
}

LINKEDLIST_TYPE *LINKEDLIST_FN_NAME(append)(LINKEDLIST_TYPE *p, LINKEDLIST_TYPE *node)
{
    if (p == NULL)
    {
        p = node;
        return p;
    }
    LINKEDLIST_TYPE *cur = p;
    while (cur->next != NULL)
    {
        cur = cur->next;
    }
    cur->next = node;
#if LINKEDLIST_DOUBLE
    node->prev = cur;
#endif
    return p;
}

#if LINKEDLIST_DOUBLE
LINKEDLIST_TYPE *LINKEDLIST_FN_NAME(remove)(LINKEDLIST_TYPE *head, LINKEDLIST_TYPE *node)
{
    LINKEDLIST_TYPE *prev = node->prev, *next = node->next;
    if (prev)
    {
        prev->next = next;
    }
    else
    {
        // This is the new first item
        head = next;
    }
    if (next)
    {
        next->prev = prev;
    }
    return head;
}
#endif

#if LINKEDLIST_DOUBLE
// From https://www.geeksforgeeks.org/insert-value-sorted-way-sorted-doubly-linked-list/
LINKEDLIST_TYPE *LINKEDLIST_FN_NAME(sortedinsert)(LINKEDLIST_TYPE *p, LINKEDLIST_TYPE *node, LINKEDLIST_FN_NAME(compare_fn) fn)
{
    LINKEDLIST_TYPE *current;

    // if list is empty
    if (p == NULL)
    {
        p = node;
    }
    else if (fn(p, node) >= 0)
    {
        // if the node is to be inserted at the beginning
        // of the doubly linked list
        node->next = p;
        node->next->prev = node;
        p = node;
    }
    else
    {
        current = p;

        // locate the node after which the new node
        // is to be inserted
        while (current->next != NULL &&
               fn(current->next, node) < 0)
            current = current->next;

        /* Make the appropriate links */
        node->next = current->next;

        // if the new node is not inserted
        // at the end of the list
        if (current->next != NULL)
            node->next->prev = node;

        current->next = node;
        node->prev = current;
    }
    return p;
}
#endif

void LINKEDLIST_FN_NAME(free)(LINKEDLIST_TYPE *head, LINKEDLIST_FN_NAME(nodefree_fn) fn)
{
    LINKEDLIST_TYPE *tmp;
    while (head != NULL)
    {
        tmp = head;
        head = head->next;
        if (fn)
        {
            fn(tmp);
        }
        else
        {
            free(tmp);
        }
    }
}
#endif