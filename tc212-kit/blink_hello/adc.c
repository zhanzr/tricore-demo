/**********************************************************************************************************************
 * \file adc.c
 * \brief VADC single-channel driver for the AN18 input (P41.6 -> VADC G1CH6).
 * \description One-shot conversion via the group scan request source.
 *********************************************************************************************************************/

#include "adc.h"
#include "IfxVadc.h"
#include "IfxVadc_Adc.h"

static IfxVadc_Adc g_vadc;
static IfxVadc_Adc_Group g_vadcGroup;
static IfxVadc_Adc_Channel g_vadcChannel;

void Adc_init(void)
{
    /* --- VADC module --- */
    IfxVadc_Adc_Config adcConfig;
    IfxVadc_Adc_initModuleConfig(&adcConfig, &MODULE_VADC);
    adcConfig.startupCalibration = FALSE;   /* only G1 is used; startup cal for all groups can hang */
    IfxVadc_Adc_initModule(&g_vadc, &adcConfig);

    /* --- Group (G1) --- */
    IfxVadc_Adc_GroupConfig adcGroupConfig;
    IfxVadc_Adc_initGroupConfig(&adcGroupConfig, &g_vadc);
    adcGroupConfig.groupId = IfxVadc_GroupId_1;
    adcGroupConfig.master  = adcGroupConfig.groupId;
    adcGroupConfig.arbiter.requestSlotScanEnabled = TRUE;
    adcGroupConfig.scanRequest.triggerConfig.gatingMode = IfxVadc_GatingMode_always;
    IfxVadc_Adc_initGroup(&g_vadcGroup, &adcGroupConfig);

    /* --- Channel CH6 of G1 (AN18, P41.6) --- */
    IfxVadc_Adc_ChannelConfig adcChannelConfig;
    IfxVadc_Adc_initChannelConfig(&adcChannelConfig, &g_vadcGroup);
    adcChannelConfig.channelId       = IfxVadc_ChannelId_6;
    adcChannelConfig.resultRegister  = IfxVadc_ChannelResult_0;
    adcChannelConfig.resultPriority  = 0;   /* polled, no interrupt */
    adcChannelConfig.inputClass      = IfxVadc_InputClasses_group0;
    IfxVadc_Adc_initChannel(&g_vadcChannel, &adcChannelConfig);
}

uint16 Adc_readRaw(void)
{
    /* One-shot scan of CH6, waiting on the result register's valid flag.
     * setScan uses ASSEL = (ASSEL & ~mask) | (channels & mask), so the mask
     * must cover the channel bits being selected. */
    uint32 ch = (uint32)1u << ADC_CHANNEL_NUM;
    IfxVadc_Adc_setScan(&g_vadcGroup, ch, ch);
    IfxVadc_Adc_startScan(&g_vadcGroup);

    Ifx_VADC_RES result;
    do
    {
        result = IfxVadc_Adc_getResult(&g_vadcChannel);
    } while (!result.B.VF);

    return (uint16)result.B.RESULT;
}

uint32 Adc_readMv(void)
{
    return (uint32)((uint32)Adc_readRaw() * ADC_VREF_MV / 4095u);
}
