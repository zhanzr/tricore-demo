/**********************************************************************************************************************
 * \file Cpu0_Main.c
 * \brief Dhrystone 2.1 benchmark for TC234 @ 200 MHz, single core (core0).
 *********************************************************************************************************************/

/*********************************************************************************************************************/
/*-----------------------------------------------------Includes------------------------------------------------------*/
/*********************************************************************************************************************/
#include "Ifx_Types.h"
#include "IfxCpu.h"
#include "IfxScuWdt.h"
#include "IfxScuCcu.h"
#include "serial.h"
#include "ticks.h"
#include "led.h"
#include "dhry.h"

/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
/*********************************************************************************************************************/
IfxCpu_syncEvent g_cpuSyncEvent = 0;

/*********************************************************************************************************************/
/*---------------------------------------------Function Implementations----------------------------------------------*/
/*********************************************************************************************************************/
int core0_main(void)
{
    IfxCpu_enableInterrupts();

    /* !!WATCHDOG0 AND SAFETY WATCHDOG ARE DISABLED HERE!!
     * Enable the watchdogs and service them periodically if it is required
     */
    IfxScuWdt_disableCpuWatchdog(IfxScuWdt_getCpuWatchdogPassword());
    IfxScuWdt_disableSafetyWatchdog(IfxScuWdt_getSafetyWatchdogPassword());

    /* Wait for CPU sync event */
    IfxCpu_emitEvent(&g_cpuSyncEvent);
    IfxCpu_waitEvent(&g_cpuSyncEvent, 1);

    initLEDs();
    initSerial();
    SystemCoreClockUpdate();
    ticks_init(0);

    float32 cpuFreq = IfxScuCcu_getCpuFrequency(IfxCpu_ResourceCpu_0);
    IfxStdIf_DPipe_print(&g_ascStandardInterface,
            "TC234 Dhrystone 2.1, CPU = %d.%02d MHz\r\n",
            (sint32)cpuFreq / 1000000, (sint32)(cpuFreq / 10000) % 100);

    dhry_main((uint32_t)cpuFreq);

    while (1)
    {
        LED1_Toggle();
        delay_ticks(500);
    }

    return (1);
}
