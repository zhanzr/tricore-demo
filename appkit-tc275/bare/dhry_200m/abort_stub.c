#include <stdlib.h>

/* newlib's libc.a in the AURIX GCC toolchain does not provide abort(),
 * but the iLLD CStart error path references it. Loop on a debug trap. */
void abort(void)
{
    while (1)
    {
        __asm volatile ("debug");
    }
}
