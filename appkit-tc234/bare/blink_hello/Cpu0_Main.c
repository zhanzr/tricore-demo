/**********************************************************************************************************************
 * \file Cpu0_Main.c
 * \copyright Copyright (C) Infineon Technologies AG 2019
 *
 * Use of this file is subject to the terms of use agreed between (i) you or the company in which ordinary course of
 * business you are acting and (ii) Infineon Technologies AG or its licensees.
 *********************************************************************************************************************/
/* \title blink_hello
 * \abstract Blink the 4 LEDs and print the CPU frequency every 2 seconds over ASC0 UART.
 * \description The 4 LEDs on P13.0 ... P13.3 (low active) are blinked with a rotating pattern and the current
 * CPU frequency is printed every 2 seconds via ASC0 (TX P14.0, RX P14.1).
 * \name blink_hello
 * \version V1.0.0
 * \board APPLICATION KIT TC2X4 V1.0, KIT_AURIX_TC234_TFT_AC-Step, TC23xLP_A-Step
 * \keywords LED, blinky, GPIO, UART, ASC, AURIX
 *********************************************************************************************************************/

/*********************************************************************************************************************/
/*-----------------------------------------------------Includes------------------------------------------------------*/
/*********************************************************************************************************************/
#include "Ifx_Types.h"
#include "IfxCpu.h"
#include "IfxScuWdt.h"
#include "IfxScuCcu.h"
#include "Bsp.h"
#include "led.h"
#include "serial.h"
#include "dts.h"

/*********************************************************************************************************************/
/*------------------------------------------------------Macros-------------------------------------------------------*/
/*********************************************************************************************************************/
#define BLINK_PERIOD_MS     200             /* LED rotation period in milliseconds    */
#define REPORT_PERIOD_MS    2000            /* CPU frequency report period in ms      */

/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
/*********************************************************************************************************************/
IfxCpu_syncEvent g_cpuSyncEvent = 0;

static Ifx_TickTime g_nextReportTime;

/*********************************************************************************************************************/
/*---------------------------------------------Function Implementations----------------------------------------------*/
/*********************************************************************************************************************/
static void reportCpuFrequency(void)
{
    float32 cpuFreq = IfxScuCcu_getCpuFrequency(IfxCpu_ResourceCpu_0);
    float32 spbFreq = IfxScuCcu_getSpbFrequency();
    float32 tempC   = read_dts_celsius();

    IfxStdIf_DPipe_print(&g_ascStandardInterface,
            "hello blink CPU=%d.%02d MHz SPB=%d.%02d MHz Die=%.2f C\r\n",
            (sint32)cpuFreq / 1000000, (sint32)(cpuFreq / 10000) % 100,
            (sint32)spbFreq / 1000000, (sint32)(spbFreq / 10000) % 100,
            tempC);
}

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

    g_nextReportTime = now();
    reportCpuFrequency();

    while (1)
    {
        blinkLEDs();
        waitTime(IfxStm_getTicksFromMilliseconds(BSP_DEFAULT_TIMER, BLINK_PERIOD_MS));

        /* Report the CPU frequency every 2 seconds */
        if (isDeadLine(g_nextReportTime))
        {
            reportCpuFrequency();
            g_nextReportTime = now();
            g_nextReportTime = g_nextReportTime + IfxStm_getTicksFromMilliseconds(BSP_DEFAULT_TIMER, REPORT_PERIOD_MS);
        }
    }

    return (1);
}
