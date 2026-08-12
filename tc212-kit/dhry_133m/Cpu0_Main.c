/**********************************************************************************************************************
 * \file Cpu0_Main.c
 * \brief Dhrystone 2.1 benchmark for TC212 @ 133 MHz, single core (core0).
 *********************************************************************************************************************/

#include "Ifx_Types.h"
#include "IfxCpu.h"
#include "IfxScuWdt.h"
#include "IfxScuCcu.h"
#include "serial.h"
#include "ticks.h"
#include "led.h"
#include "dhry.h"
#include "dts.h"

IfxCpu_syncEvent g_cpuSyncEvent = 0;

int core0_main(void)
{
    IfxCpu_enableInterrupts();

    IfxScuWdt_disableCpuWatchdog(IfxScuWdt_getCpuWatchdogPassword());
    IfxScuWdt_disableSafetyWatchdog(IfxScuWdt_getSafetyWatchdogPassword());

    IfxCpu_emitEvent(&g_cpuSyncEvent);
    IfxCpu_waitEvent(&g_cpuSyncEvent, 1);

    initLEDs();
    initSerial();
    SystemCoreClockUpdate();
    ticks_init(0);

    float32 cpuFreq = IfxScuCcu_getCpuFrequency(IfxCpu_ResourceCpu_0);
    float32 tempC   = read_dts_celsius();
    PRINTF("TC212 Dhrystone 2.1, CPU = %d.%02d MHz, Die = %.2f C\r\n",
            (sint32)cpuFreq / 1000000, (sint32)(cpuFreq / 10000) % 100,
            tempC);

    dhry_main((uint32_t)cpuFreq);

    while (1)
    {
        static uint8 led = 0;
        toggleLED(led);
        led = (uint8)((led + 1) % LED_COUNT);
        delay_ticks(500);
    }

    return 1;
}
