/**********************************************************************************************************************
 * \file Cpu0_Main.c
 * \brief PWM passive buzzer test on P33.0 @ 2048 Hz.
 * \description Sweeps duty cycle 0 -> 100%, holds, 100 -> 0%, holds, loops.
 *********************************************************************************************************************/

#include "Ifx_Types.h"
#include "IfxCpu.h"
#include "IfxScuWdt.h"
#include "Bsp.h"
#include "buzzer.h"
#include "serial.h"
#include "dts.h"

IfxCpu_syncEvent g_cpuSyncEvent = 0;

#define STEP_DELAY_MS   100u
#define HOLD_DELAY_MS   1000u

static void delayMs(uint32 ms)
{
    waitTime(IfxStm_getTicksFromMilliseconds(BSP_DEFAULT_TIMER, ms));
}

int core0_main(void)
{
    IfxCpu_enableInterrupts();

    IfxScuWdt_disableCpuWatchdog(IfxScuWdt_getCpuWatchdogPassword());
    IfxScuWdt_disableSafetyWatchdog(IfxScuWdt_getSafetyWatchdogPassword());

    IfxCpu_emitEvent(&g_cpuSyncEvent);
    IfxCpu_waitEvent(&g_cpuSyncEvent, 1);

    initSerial();
    Buzzer_init();

    float32 tempC = read_dts_celsius();
    IfxStdIf_DPipe_print(&g_ascStandardInterface,
            "pwm_buzz_test: P33.0 TOM0_CH4 2048 Hz, sweep duty 0-100-0, Die = %.2f C\r\n",
            tempC);

    while (1)
    {
        /* 0 -> 100 */
        uint8 d;
        for (d = 0; d <= 100; d += 10)
        {
            Buzzer_setDutyCycle(d);
            delayMs(STEP_DELAY_MS);
        }

        /* hold at 100% */
        delayMs(HOLD_DELAY_MS);

        /* 100 -> 0 */
        for (d = 100; d > 0; d -= 10)
        {
            Buzzer_setDutyCycle(d);
            delayMs(STEP_DELAY_MS);
        }

        /* hold silent */
        Buzzer_stop();
        delayMs(HOLD_DELAY_MS);
    }

    return 1;
}
