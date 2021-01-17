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

static int linkedlist_index(LINKEDLIST_TYPE *p, LINKEDLIST_TYPE *f)
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
typedef int(LINKEDLIST_COMPARE_FN)(LINKEDLIST_TYPE *p1, LINKEDLIST_TYPE *p2);

// From https://www.geeksforgeeks.org/insert-value-sorted-way-sorted-doubly-linked-list/
static LINKEDLIST_TYPE *linkedlist_sortedinsert(LINKEDLIST_TYPE *p, LINKEDLIST_TYPE *node, LINKEDLIST_COMPARE_FN fn)
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