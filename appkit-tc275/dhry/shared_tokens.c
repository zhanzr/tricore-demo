#include "shared_tokens.h"

/* The token must be in RAM that ALL cores can access. On TC27x the DSPRs are
 * globally addressable (dsram1 = 0x60000000 is accessible from every core via
 * its global alias). Placing it in LMU (.bss_lmu at 0x90000000) breaks the
 * startup of cores 1/2 in GCC builds, so we use the default .bss (dsram1).
 * TASKING keeps its original LMU placement. */
#if defined(__TASKING__)
#pragma section farbss "lmu_sram"
volatile uint32_t g_activeCoreToken = 0; // 0 = Core0, 1 = Core1, 2 = Core2
#pragma section farbss restore
#else
volatile uint32_t g_activeCoreToken = 0; // 0 = Core0, 1 = Core1, 2 = Core2
#endif

