#include <stdlib.h>

void abort(void)
{
    while (1)
    {
        __asm volatile ("debug");
    }
}

/* Older iLLD CStart references _init() (C/C++ runtime init); newer GCC does
 * not link it with -nostdlib. Provide empty stubs. */
void _init(void) { }
void _fini(void) { }
