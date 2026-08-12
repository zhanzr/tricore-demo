#include "ticks.h"
#include "Compilers.h" // TASKING intrinsic definitions

static Ifx_STM * const g_stmModule = &MODULE_STM0;
static uint32_t g_ticksPerMs = 100000;


uint32_t SystemCoreClock = 200000000U; /* Default 200 MHz for TC27x */

void SystemCoreClockUpdate(void)
{
//    SystemCoreClock = (uint32_t)IfxScuCcu_getCpuFrequency(IfxCpu_getCoreId());
    // SRI bus runs at CPU clock frequency (typically 200 MHz)
    SystemCoreClock = (uint32_t)IfxScuCcu_getSriFrequency();
}

void ticks_init(uint32_t sys_freq)
{
    if (sys_freq == 0)
    {
        sys_freq = (uint32_t)IfxStm_getFrequency(g_stmModule);
    }
    g_ticksPerMs = sys_freq / 1000U;
}

uint32_t get_tick_count(void)
{
    // Fetch full 64-bit hardware timer count to prevent 42.9s rollover
    uint64_t raw_ticks_64 = (uint64_t)IfxStm_get(g_stmModule);
    
    // Return 32-bit millisecond count (will overflow naturally after ~49.7 days)
    return (uint32_t)(raw_ticks_64 / g_ticksPerMs);
}

void delay_ticks(uint32_t ticks)
{
    if (ticks == 0) return;

    uint64_t delayHardwareTicks = (uint64_t)ticks * g_ticksPerMs;
    uint64_t startTicks = (uint64_t)IfxStm_get(g_stmModule);

    while (((uint64_t)IfxStm_get(g_stmModule) - startTicks) < delayHardwareTicks)
    {
        // Blocking wait using 64-bit timer reads
    }
}


void get_raw_core_info(uint32 *out_core_id, uint32 *out_cpu_id)
{
    // __mfcr (Move From Core Register) reads CSFR registers
    // 0xFE1C = CORE_ID register
    // 0xFE18 = CPU_ID register
    *out_core_id = __mfcr(0xFE1C) & 0x0F; // Extract lower 4 bits for Core ID
    *out_cpu_id  = __mfcr(0xFE18);        // Architecture/Revision details
}
