#include <stdarg.h>
#include <stdio.h>

#include <stdint.h>
#include <stdbool.h>

#include "ticks.h"
#include "utils.h"

uint32_t HAL_GetTick(void) {
	return get_tick_count();
}

void HAL_Delay(uint32_t t) {
    delay_ticks(t);
}
