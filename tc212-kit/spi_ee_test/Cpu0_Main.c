/**********************************************************************************************************************
 * \file Cpu0_Main.c
 * \brief AT25128N SPI EEPROM erase/program/read speed test (TC212, QSPI0).
 * \description Pins: CS=P33.5, SCLK=P20.11, MOSI=P20.14, MISO=P20.12.
 * Measures and prints full-memory erase (0xFF fill), program (pattern) and
 * read timings over ASC0 UART.
 *********************************************************************************************************************/

#include "Ifx_Types.h"
#include "IfxCpu.h"
#include "IfxScuWdt.h"
#include "IfxScuCcu.h"
#include "IfxStm.h"
#include "serial.h"
#include "dts.h"
#include "ee.h"
#include "string.h"

IfxCpu_syncEvent g_cpuSyncEvent = 0;

static uint64 g_stmFreq;

static uint64 now_us(void)
{
    uint64 ticks = IfxStm_get(&MODULE_STM0);
    return (uint64)(ticks * 1000000ull / g_stmFreq);
}

static void report_speed(const char *label, uint32 bytes, uint64 us)
{
    float32 mbps = (float32)bytes / (float32)us;   /* bytes/us == MB/s */
    PRINTF("%-26s %5u B: %8.3f ms, %8.3f MB/s\r\n",
            label, bytes, (float32)us / 1000.0f, mbps);
}

int core0_main(void)
{
    IfxCpu_enableInterrupts();

    IfxScuWdt_disableCpuWatchdog(IfxScuWdt_getCpuWatchdogPassword());
    IfxScuWdt_disableSafetyWatchdog(IfxScuWdt_getSafetyWatchdogPassword());

    IfxCpu_emitEvent(&g_cpuSyncEvent);
    IfxCpu_waitEvent(&g_cpuSyncEvent, 1);

    initSerial();
    g_stmFreq = IfxStm_getFrequency(&MODULE_STM0);
    EE_init();

    float32 cpuFreq = IfxScuCcu_getCpuFrequency(IfxCpu_ResourceCpu_0);
    float32 tempC   = read_dts_celsius();
    PRINTF("TC212 AT25128N SPI EEPROM test, CPU=%d.%02d MHz, Die=%.2f C\r\n",
            (sint32)cpuFreq / 1000000, (sint32)(cpuFreq / 10000) % 100, tempC);
    PRINTF("QSPI0 2 MHz: CS=P33.5 SCLK=P20.11 MOSI=P20.14 MISO=P20.12\r\n");
    PRINTF("STATUS: WIP=%u WEL=%u\r\n", EE_readStatus() & EE_SR_WIP, (EE_readStatus() & EE_SR_WEL) != 0);

    uint8 page[EE_PAGE_SIZE];
    uint8 rd[EE_PAGE_SIZE];
    uint16 addr;
    uint64 t0, t1, us;
    uint32 i, p;

    /* --- 1. Erase: write 0xFF to every page --- */
    memset(page, 0xFF, EE_PAGE_SIZE);
    t0 = now_us();
    for (p = 0; p < EE_NUM_PAGES; p++)
    {
        EE_pageWrite((uint16)(p * EE_PAGE_SIZE), page, EE_PAGE_SIZE);
    }
    t1 = now_us();
    us = t1 - t0;
    report_speed("erase (write 0xFF)", EE_SIZE_BYTES, us);
    PRINTF("  avg %.2f us/page, %.1f us/byte\r\n", (float32)us / EE_NUM_PAGES, (float32)us / EE_SIZE_BYTES);

    /* --- 2. Program: write an incrementing pattern to every page --- */
    t0 = now_us();
    for (p = 0; p < EE_NUM_PAGES; p++)
    {
        for (i = 0; i < EE_PAGE_SIZE; i++)
        {
            page[i] = (uint8)(((p * EE_PAGE_SIZE) + i) & 0xFF);
        }
        EE_pageWrite((uint16)(p * EE_PAGE_SIZE), page, EE_PAGE_SIZE);
    }
    t1 = now_us();
    us = t1 - t0;
    report_speed("program (pattern)", EE_SIZE_BYTES, us);
    PRINTF("  avg %.2f us/page, %.1f us/byte\r\n", (float32)us / EE_NUM_PAGES, (float32)us / EE_SIZE_BYTES);

    /* --- 3. Read back and verify --- */
    boolean ok = TRUE;
    uint32 badFirst = 0;
    t0 = now_us();
    for (p = 0; p < EE_NUM_PAGES; p++)
    {
        addr = (uint16)(p * EE_PAGE_SIZE);
        EE_read(addr, rd, EE_PAGE_SIZE);
        for (i = 0; i < EE_PAGE_SIZE; i++)
        {
            uint8 expect = (uint8)((addr + i) & 0xFF);
            if (rd[i] != expect)
            {
                if (ok)
                {
                    badFirst = addr + i;
                }
                ok = FALSE;
            }
        }
    }
    t1 = now_us();
    us = t1 - t0;
    report_speed("read", EE_SIZE_BYTES, us);

    if (ok)
    {
        PRINTF("verify: OK, all %u bytes match\r\n", (unsigned)EE_SIZE_BYTES);
    }
    else
    {
        PRINTF("verify: FAIL, first mismatch at 0x%04X\r\n", (unsigned)badFirst);
    }

    PRINTF("done\r\n");

    while (1)
    {
    }

    return 1;
}
