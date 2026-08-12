/**********************************************************************************************************************
 * \file adc.h
 * \brief VADC single-channel driver for the AN18 input (P41.6 -> VADC G1CH6).
 *********************************************************************************************************************/

#ifndef ADC_H_
#define ADC_H_

#include "Ifx_Types.h"

/* AN18 on the TC212 Application Kit -> VADC group 1, channel 6 (P41.6). */
#define ADC_CHANNEL_NUM     6u
#define ADC_GROUP_ID        IfxVadc_GroupId_1
#define ADC_VREF_MV         3300u   /* Varef = board supply (3.3 V) */

void Adc_init(void);
uint16 Adc_readRaw(void);          /* 12-bit raw conversion value 0..4095 */
uint32 Adc_readMv(void);           /* scaled voltage in mV */

#endif /* ADC_H_ */
