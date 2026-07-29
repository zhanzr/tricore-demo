#ifndef SHARED_TOKENS_H
#define SHARED_TOKENS_H

#include <stdint.h>
#include "Ifx_Types.h" // Provides __nop() or compiler intrinsics

// Declare variable as extern so all cores see it without duplicating symbols
extern volatile uint32_t g_activeCoreToken;

// Helper macro for barrier wait
#define WAIT_FOR_MY_TURN(core_id) while (g_activeCoreToken != (core_id)) { __nop(); }

#endif /* SHARED_TOKENS_H */

