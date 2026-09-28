/**********************************************************************************************************************
 * \file Cpu0_Main.c
 * \copyright Copyright (C) Infineon Technologies AG 2019
 *
 * Boost Software License - Version 1.0 (see notice in other project files).
 *********************************************************************************************************************/
 /*\title NV3030B 240x284 LCD + CST816D touch (QSPI1 + soft I2C)
 * \description Drives the TK018F3716 240x284 touch LCD module: NV3030B panel
 *              over QSPI1 (wrapped-command 8-bit SPI, SCLK=P11.6 MOSI=P11.9
 *              CS=P11.2) and CST816D touch over bit-banged I2C (SDA=P21.4,
 *              SCL=P21.2). Demo patterns and touch state are printed on the
 *              ASC0 serial console.
 * \name nv3030b_md183_240x284_cst816d
 * \board TC212 Application Kit
 * \keywords QSPI, I2C, LCD, NV3030B, CST816D, touch, AURIX
 *********************************************************************************************************************/
#include "Ifx_Types.h"
#include "IfxCpu.h"
#include "IfxScuWdt.h"
#include "serial.h"
#include "IfxAsclin_Asc.h"

#include "Bsp.h"

IfxCpu_syncEvent g_cpuSyncEvent = 0;

extern void lcd_demo_main(void);

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

    initSerial();

    /* Run the LCD + touch demo (never returns) */
    lcd_demo_main();

    return (1);
}
