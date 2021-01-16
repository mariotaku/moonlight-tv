#pragma once
#ifndef LINKEDLIST_TYPE
#error "Please define LINKEDLIST_TYPE before include"
#endif

#include <stdlib.h>
#include <string.h>

static LINKEDLIST_TYPE *_linkedlist_new(size_t size)
{
    LINKEDLIST_TYPE *node = malloc(size);
#if LINKEDLIST_DOUBLE
    node->prev = NULL;
#endif
    node->next = NULL;
    return node;
}

#define linkedlist_new() _linkedlist_new(sizeof(LINKEDLIST_TYPE))

static int linkedlist_len(LINKEDLIST_TYPE *p)
{
    int length = 0;
    LINKEDLIST_TYPE *cur = p;
    while (cur != NULL)
    {
        length++;
        cur = cur->next;
    }
    return length;
}

static LINKEDLIST_TYPE *linkedlist_nth(LINKEDLIST_TYPE *p, int n)
{
    LINKEDLIST_TYPE *ret = NULL;
    int i = 0;
    for (ret = p; ret != NULL && i < n; ret = ret->next, i++)
        ;
    return ret;
}

typedef int(LINKEDLIST_FIND_FN)(LINKEDLIST_TYPE *p, const void *fv);

static LINKEDLIST_TYPE *linkedlist_find_by(LINKEDLIST_TYPE *p, const void *v, LINKEDLIST_FIND_FN fn)
{
    LINKEDLIST_TYPE *ret = NULL;
    int i = 0;
    for (ret = p; ret != NULL && fn(ret, v) != 0; ret = ret->next, i++)
        ;
    return ret;
}

static LINKEDLIST_TYPE *linkedlist_append(LINKEDLIST_TYPE *p, LINKEDLIST_TYPE *node)
{
    if (p == NULL)
    {
        p = node;
        return p;
    }
    LINKEDLIST_TYPE *cur = p;
    while (cur->next != NULL)
        ;
    cur->next = node;
#if LINKEDLIST_DOUBLE
    node->prev = cur;
#endif
    return p;
}

static void linkedlist_free(LINKEDLIST_TYPE *head)
{
    LINKEDLIST_TYPE *tmp;
    while (head != NULL)
    {
        tmp = head;
        head = head->next;
        free(tmp);
    }
}