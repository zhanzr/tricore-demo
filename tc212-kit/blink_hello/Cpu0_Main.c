/**********************************************************************************************************************
 * \file Cpu0_Main.c
 * \brief TC212 blink_hello: blink 8 LEDs, print CPU frequency + die temperature over ASC0 UART.
 * \description LEDs: P02.0..P02.5, P11.10, P11.11 (low active).
 * UART: ASC0, 115200 8N1.
 *********************************************************************************************************************/

#include "Ifx_Types.h"
#include "IfxCpu.h"
#include "IfxScuWdt.h"
#include "IfxScuCcu.h"
#include "IfxScu_reg.h"
#include "Bsp.h"
#include "led.h"
#include "serial.h"
#include "dts.h"
#include "adc.h"

#define BLINK_PERIOD_MS     200
#define REPORT_PERIOD_MS    2000

IfxCpu_syncEvent g_cpuSyncEvent = 0;
static Ifx_TickTime g_nextReportTime;

static void report(void)
{
    float32 cpuFreq = IfxScuCcu_getCpuFrequency(IfxCpu_ResourceCpu_0);
    float32 spbFreq = IfxScuCcu_getSpbFrequency();
    float32 tempC   = read_dts_celsius();
    uint16  anRaw   = Adc_readRaw();
    uint32  anMv    = (uint32)anRaw * ADC_VREF_MV / 4095u;

    PRINTF("TC212 blink CPU=%d.%02d MHz SPB=%d.%02d MHz Die=%.2f C AN18=%u.%03u V (raw %u)\r\n",
            (sint32)cpuFreq / 1000000, (sint32)(cpuFreq / 10000) % 100,
            (sint32)spbFreq / 1000000, (sint32)(spbFreq / 10000) % 100,
            tempC, anMv / 1000u, anMv % 1000u, anRaw);

    /* Diagnostic: actual silicon identifiers */
    volatile Ifx_SCU_CHIPID chipid = MODULE_SCU.CHIPID;
    PRINTF("CHIPID: CHID=%02X CHREV=%d FSIZE=%d EEA=%d SP=%d SEC=%d\r\n",
            (unsigned)chipid.B.CHID, (unsigned)chipid.B.CHREV,
            (unsigned)chipid.B.FSIZE, (unsigned)chipid.B.EEA,
            (unsigned)chipid.B.SP, (unsigned)chipid.B.SEC);
}

int core0_main(void)
{
    IfxCpu_enableInterrupts();

    IfxScuWdt_disableCpuWatchdog(IfxScuWdt_getCpuWatchdogPassword());
    IfxScuWdt_disableSafetyWatchdog(IfxScuWdt_getSafetyWatchdogPassword());

    IfxCpu_emitEvent(&g_cpuSyncEvent);
    IfxCpu_waitEvent(&g_cpuSyncEvent, 1);

    initLEDs();
    initSerial();
    Adc_init();

    g_nextReportTime = now();
    report();

    while (1)
    {
        uint8 i;
        for (i = 0; i < LED_COUNT; i++)
        {
            setLED(i, TRUE);
            waitTime(IfxStm_getTicksFromMilliseconds(BSP_DEFAULT_TIMER, BLINK_PERIOD_MS));
            setLED(i, FALSE);
        }

        if (isDeadLine(g_nextReportTime))
        {
            report();
            g_nextReportTime = now();
            g_nextReportTime = g_nextReportTime + IfxStm_getTicksFromMilliseconds(BSP_DEFAULT_TIMER, REPORT_PERIOD_MS);
        }
    }

    return 1;
}
