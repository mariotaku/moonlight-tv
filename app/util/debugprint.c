#include "debugprint.h"

#include <assert.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void print_bytes(const void *ptr, int size)
{
    const unsigned char *p = ptr;
    int i;
    for (i = 0; i < size; i++)
    {
        printf("%02hhX ", p[i]);
    }
    printf("\n");
}