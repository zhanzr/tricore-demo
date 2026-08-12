/**********************************************************************************************************************
 * \file serial.h
 * \brief ASC0 UART for TC212.
 * \description Two candidate pin pairs on the board:
 *   - COMBO 1: TX P15.2, RX P15.3  (IfxAsclin0_TX_P15_2_OUT, IfxAsclin0_RXB_P15_3_IN)
 *   - COMBO 2: TX P14.0, RX P14.1  (IfxAsclin0_TX_P14_0_OUT, IfxAsclin0_RXA_P14_1_IN)
 * Select with SERIAL_UART_COMBO below.
 *********************************************************************************************************************/

#ifndef SERIAL_H_
#define SERIAL_H_

#include "IfxStdIf_DPipe.h"

#ifndef PRINTF
#define PRINTF(...) IfxStdIf_DPipe_print(&g_ascStandardInterface, __VA_ARGS__)
#endif

#define SERIAL_UART_COMBO   1    /* 1 = P15.2/P15.3, 2 = P14.0/P14.1 */

extern IfxStdIf_DPipe g_ascStandardInterface;

void initSerial(void);

#endif /* SERIAL_H_ */
