/**********************************************************************************************************************
 * \file serial.c
 * \copyright Copyright (C) Infineon Technologies AG 2019
 *
 * Use of this file is subject to the terms of use agreed between (i) you or the company in which ordinary course of
 * business you are acting and (ii) Infineon Technologies AG or its licensees.
 *********************************************************************************************************************/

/*********************************************************************************************************************/
/*-----------------------------------------------------Includes------------------------------------------------------*/
/*********************************************************************************************************************/
#include "IfxAsclin_Asc.h"
#include "IfxPort.h"
#include "IfxSrc.h"
#include "serial.h"

/*********************************************************************************************************************/
/*------------------------------------------------------Macros-------------------------------------------------------*/
/*********************************************************************************************************************/
#define ASC_BAUDRATE            115200
#define ASC_TX_BUFFER_SIZE      256
#define ASC_RX_BUFFER_SIZE      256
#define ISR_PRIORITY_ASCLIN_TX  8
#define ISR_PRIORITY_ASCLIN_RX  4
#define ISR_PRIORITY_ASCLIN_ER  12

/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
/*********************************************************************************************************************/
IfxStdIf_DPipe g_ascStandardInterface;
IfxAsclin_Asc g_asclin;

/* Ifx_Fifo_init() casts these buffers to Ifx_Fifo* and the FIFO code performs
 * 32-bit accesses at offsets 0/4/12 of that struct, so the buffers MUST be
 * 4-byte aligned. Declared as plain uint8[] the linker only guarantees
 * alignment 1, and higher optimisation levels (e.g. -Ofast) then emit wide
 * accesses that raise an instruction-error trap on a misaligned address
 * (trap class 2, tin 4). Keep the explicit alignment. */
uint8 g_uartTxBuffer[ASC_TX_BUFFER_SIZE + sizeof(Ifx_Fifo) + 8] __attribute__((aligned(4)));
uint8 g_uartRxBuffer[ASC_RX_BUFFER_SIZE + sizeof(Ifx_Fifo) + 8] __attribute__((aligned(4)));

/* Set once initSerial() is done. The ISRs can fire as soon as the ASC
 * SRCs are enabled inside IfxAsclin_Asc_initModule - BEFORE the DPipe
 * interface is initialized - and IfxStdIf_DPipe_onReceive/onTransmit
 * would call through a NULL pointer (trap -> halted CPU). */
static volatile boolean g_serialReady = FALSE;

/*********************************************************************************************************************/
/*---------------------------------------------Function Implementations----------------------------------------------*/
/*********************************************************************************************************************/
IFX_INTERRUPT(asc0TxISR, 0, ISR_PRIORITY_ASCLIN_TX);

void asc0TxISR(void)
{
    if (g_serialReady)
    {
        IfxStdIf_DPipe_onTransmit(&g_ascStandardInterface);
    }
}

IFX_INTERRUPT(asc0RxISR, 0, ISR_PRIORITY_ASCLIN_RX);

void asc0RxISR(void)
{
    if (g_serialReady)
    {
        IfxStdIf_DPipe_onReceive(&g_ascStandardInterface);
    }
    else
    {
        /* spurious byte during init: drain the RX FIFO so the request
         * does not retrigger */
        IfxAsclin_flushRxFifo(g_asclin.asclin);
    }
}

IFX_INTERRUPT(asc0ErrISR, 0, ISR_PRIORITY_ASCLIN_ER);

void asc0ErrISR(void)
{
    if (g_serialReady)
    {
        IfxStdIf_DPipe_onError(&g_ascStandardInterface);
    }
    else if (g_asclin.asclin != NULL_PTR)
    {
        IfxAsclin_Asc_isrError(&g_asclin);
    }
}

void initSerial(void)
{
    IfxAsclin_Asc_Config ascConf;

    IfxAsclin_Asc_initModuleConfig(&ascConf, &MODULE_ASCLIN0);

    ascConf.baudrate.baudrate = ASC_BAUDRATE;
    ascConf.baudrate.oversampling = IfxAsclin_OversamplingFactor_16;

    ascConf.bitTiming.medianFilter = IfxAsclin_SamplesPerBit_three;
    ascConf.bitTiming.samplePointPosition = IfxAsclin_SamplePointPosition_8;

    ascConf.interrupt.txPriority = ISR_PRIORITY_ASCLIN_TX;
    ascConf.interrupt.rxPriority = ISR_PRIORITY_ASCLIN_RX;
    ascConf.interrupt.erPriority = ISR_PRIORITY_ASCLIN_ER;
    ascConf.interrupt.typeOfService = IfxSrc_Tos_cpu0;

    const IfxAsclin_Asc_Pins pins = {
        .cts = NULL_PTR,
        .ctsMode = IfxPort_InputMode_pullUp,
        .rx = &IfxAsclin0_RXA_P14_1_IN,       /* ASC0 RX on P14.1 */
        .rxMode = IfxPort_InputMode_pullUp,
        .rts = NULL_PTR,
        .rtsMode = IfxPort_OutputMode_pushPull,
        .tx = &IfxAsclin0_TX_P14_0_OUT,       /* ASC0 TX on P14.0 */
        .txMode = IfxPort_OutputMode_pushPull,
        .pinDriver = IfxPort_PadDriver_cmosAutomotiveSpeed1
    };
    ascConf.pins = &pins;

    ascConf.txBuffer = g_uartTxBuffer;
    ascConf.txBufferSize = ASC_TX_BUFFER_SIZE;
    ascConf.rxBuffer = g_uartRxBuffer;
    ascConf.rxBufferSize = ASC_RX_BUFFER_SIZE;

    IfxAsclin_Asc_initModule(&g_asclin, &ascConf);
    IfxAsclin_Asc_stdIfDPipeInit(&g_ascStandardInterface, &g_asclin);
    g_serialReady = TRUE;
}
