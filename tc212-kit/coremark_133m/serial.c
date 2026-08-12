/**********************************************************************************************************************
 * \file serial.c
 * \brief ASC0 UART for TC212.
 *********************************************************************************************************************/

#include "IfxAsclin_Asc.h"
#include "IfxPort.h"
#include "serial.h"

#define ASC_BAUDRATE            115200
#define ASC_TX_BUFFER_SIZE      256
#define ASC_RX_BUFFER_SIZE      256
#define ISR_PRIORITY_ASCLIN_TX  8
#define ISR_PRIORITY_ASCLIN_RX  4
#define ISR_PRIORITY_ASCLIN_ER  12

IfxStdIf_DPipe g_ascStandardInterface;
static IfxAsclin_Asc g_asclin;
static uint8 g_uartTxBuffer[ASC_TX_BUFFER_SIZE + sizeof(Ifx_Fifo) + 8];
static uint8 g_uartRxBuffer[ASC_RX_BUFFER_SIZE + sizeof(Ifx_Fifo) + 8];

IFX_INTERRUPT(asc0TxISR, 0, ISR_PRIORITY_ASCLIN_TX);

void asc0TxISR(void)
{
    IfxStdIf_DPipe_onTransmit(&g_ascStandardInterface);
}

IFX_INTERRUPT(asc0RxISR, 0, ISR_PRIORITY_ASCLIN_RX);

void asc0RxISR(void)
{
    IfxStdIf_DPipe_onReceive(&g_ascStandardInterface);
}

IFX_INTERRUPT(asc0ErrISR, 0, ISR_PRIORITY_ASCLIN_ER);

void asc0ErrISR(void)
{
    IfxStdIf_DPipe_onError(&g_ascStandardInterface);
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

#if (SERIAL_UART_COMBO == 1)
    /* TX P15.2, RX P15.3 */
    const IfxAsclin_Asc_Pins pins = {
        .cts = NULL_PTR,
        .ctsMode = IfxPort_InputMode_pullUp,
        .rx = &IfxAsclin0_RXB_P15_3_IN,     /* RX on P15.3 */
        .rxMode = IfxPort_InputMode_pullUp,
        .rts = NULL_PTR,
        .rtsMode = IfxPort_OutputMode_pushPull,
        .tx = &IfxAsclin0_TX_P15_2_OUT,     /* TX on P15.2 */
        .txMode = IfxPort_OutputMode_pushPull,
        .pinDriver = IfxPort_PadDriver_cmosAutomotiveSpeed1
    };
#else
    /* TX P14.0, RX P14.1 */
    const IfxAsclin_Asc_Pins pins = {
        .cts = NULL_PTR,
        .ctsMode = IfxPort_InputMode_pullUp,
        .rx = &IfxAsclin0_RXA_P14_1_IN,     /* RX on P14.1 */
        .rxMode = IfxPort_InputMode_pullUp,
        .rts = NULL_PTR,
        .rtsMode = IfxPort_OutputMode_pushPull,
        .tx = &IfxAsclin0_TX_P14_0_OUT,     /* TX on P14.0 */
        .txMode = IfxPort_OutputMode_pushPull,
        .pinDriver = IfxPort_PadDriver_cmosAutomotiveSpeed1
    };
#endif
    ascConf.pins = &pins;

    ascConf.txBuffer = g_uartTxBuffer;
    ascConf.txBufferSize = ASC_TX_BUFFER_SIZE;
    ascConf.rxBuffer = g_uartRxBuffer;
    ascConf.rxBufferSize = ASC_RX_BUFFER_SIZE;

    IfxAsclin_Asc_initModule(&g_asclin, &ascConf);
    IfxStdIf_DPipe_ascInit(&g_ascStandardInterface, &g_asclin);
}
