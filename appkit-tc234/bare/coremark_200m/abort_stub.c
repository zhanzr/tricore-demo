#include <stdlib.h>

void abort(void)
{
    while (1)
    {
        /* trap / halt */
        __asm volatile ("debug");
    }
}
