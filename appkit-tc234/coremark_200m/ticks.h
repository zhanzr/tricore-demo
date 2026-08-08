#ifndef TICKS_H
#define TICKS_H

#include <stdint.h>
#include <inttypes.h>

#include "Ifx_Types.h"
#include "IfxStm.h"
#include "IfxScuCcu.h"
#include "IfxCpu.h"

extern uint32_t SystemCoreClock; /* Global CPU Clock in Hz */

void SystemCoreClockUpdate(void);

/**
 * @brief Initializes the tick timer subsystem.
 * @param sys_freq System STM frequency in Hz (pass 0 to auto-detect via IfxStm_getFrequency).
 */
void ticks_init(uint32_t sys_freq);

/**
 * @brief Gets the current tick count in milliseconds since initialization.
 * @return Monotonic millisecond counter (32-bit rollover after ~49.7 days).
 */
uint32_t get_tick_count(void);

/**
 * @brief Delays execution for the specified number of milliseconds (blocking).
 * @param ticks Number of milliseconds to delay.
 */
void delay_ticks(uint32_t ticks);

void get_raw_core_info(uint32 *out_core_id, uint32 *out_cpu_id);

#endif /* TICKS_H */
