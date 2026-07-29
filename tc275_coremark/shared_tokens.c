#include "shared_tokens.h"

// Allocate and initialize actual memory in shared LMU RAM ONCE
#pragma section farbss "lmu_sram"
volatile uint32_t g_activeCoreToken = 0; // 0 = Core0, 1 = Core1, 2 = Core2
#pragma section farbss restore

