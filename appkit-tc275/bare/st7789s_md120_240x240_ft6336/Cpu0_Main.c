/**********************************************************************************************************************
 * \file Cpu0_Main.c
 * \copyright Copyright (C) Infineon Technologies AG 2019
 *
 * Use of this file is subject to the terms of use agreed between (i) you or the company in which ordinary course of
 * business you are acting and (ii) Infineon Technologies AG or its licensees. If and as long as no such terms of use
 * are agreed, use of this file is subject to following:
 *
 * Boost Software License - Version 1.0 - August 17th, 2003
 *
 * Permission is hereby granted, free of charge, to any person or organization obtaining a copy of the software and
 * accompanying documentation covered by this license (the "Software"), to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the Software, and to permit third parties to
 * whom the Software is furnished to do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including the above license grant, this restriction
 * and the following disclaimer, must be included in all copies of the Software, in whole or in part, and all
 * derivative works of the Software, unless the source code is modified.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *********************************************************************************************************************/
 /*\title ST7789S 240x240 LCD + FT6336 touch (QSPI2 + I2C0)
 * \description Drives the TK012F6 240x240 touch LCD module: ST7789S panel over QSPI2
 *              (3-wire 9-bit serial) and FT6336 touch over I2C0. Demo patterns and
 *              touch state are printed on the ASCLIN0 (ASC0) serial console.
 * \name st7789s_md120_240x240_ft6336
 * \board APPLICATION KIT TC2x5 V2.0, KIT_AURIX_TC275_TFT-CA-Step, TC27xTP_D-Step
 * \keywords QSPI, I2C, LCD, ST7789S, FT6336, touch, AURIX
 *********************************************************************************************************************/
#include "Ifx_Types.h"
#include "IfxCpu.h"
#include "IfxScuWdt.h"
#include "ASCLIN_Shell_UART.h"
#include "IfxAsclin_Asc.h"

#include "ticks.h"
#include "led.h"

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

    SystemCoreClockUpdate();
    /* Initialize ticks system (pass 0 to auto-detect clock speed) */
    ticks_init(0);

    /* Initialize the Shell Interface and the UART communication */
    initShellInterface();

    initLEDs();

    /* Run the LCD + touch demo (never returns) */
    lcd_demo_main();

    return (1);
}
