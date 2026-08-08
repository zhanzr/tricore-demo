/**********************************************************************************************************************
 * \file led.c
 * \copyright Copyright (C) Infineon Technologies AG 2019
 *
 * Use of this file is subject to the terms of use agreed between (i) you or the company in which ordinary course of
 * business you are acting and (ii) Infineon Technologies AG or its licensees.
 *********************************************************************************************************************/

/*********************************************************************************************************************/
/*-----------------------------------------------------Includes------------------------------------------------------*/
/*********************************************************************************************************************/
#include "IfxPort.h"
#include "led.h"

/*********************************************************************************************************************/
/*------------------------------------------------------Macros-------------------------------------------------------*/
/*********************************************************************************************************************/
#define LED_COUNT   4
#define LED_START_PIN 0

/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
/*********************************************************************************************************************/
/* The 4 LEDs are connected to P13.0 ... P13.3, low active */
static const Ifx_P *g_ledPort = &MODULE_P13;
static uint8 g_ledPattern = 0x01;      /* initial pattern, low active */

/*********************************************************************************************************************/
/*---------------------------------------------Function Implementations----------------------------------------------*/
/*********************************************************************************************************************/
void initLEDs(void)
{
    uint8 i;

    for (i = 0; i < LED_COUNT; i++)
    {
        IfxPort_setPinModeOutput(g_ledPort, LED_START_PIN + i, IfxPort_OutputMode_pushPull, IfxPort_OutputIdx_general);
        IfxPort_setPinHigh(g_ledPort, LED_START_PIN + i);   /* switch OFF the LED (low-level active) */
    }
}

void blinkLEDs(void)
{
    uint8 i;

    g_ledPattern = (uint8)((g_ledPattern << 1) | (g_ledPattern >> (LED_COUNT - 1)));  /* rotate left */
    g_ledPattern = (uint8)(g_ledPattern & 0x0F);

    for (i = 0; i < LED_COUNT; i++)
    {
        boolean on = ((g_ledPattern >> i) & 0x01) ? TRUE : FALSE;
        IfxPort_setPinState(g_ledPort, LED_START_PIN + i, on ? IfxPort_State_low : IfxPort_State_high);
    }
}

void setLEDs(uint8 pattern)
{
    uint8 i;

    g_ledPattern = pattern & 0x0F;
    for (i = 0; i < LED_COUNT; i++)
    {
        boolean on = ((g_ledPattern >> i) & 0x01) ? TRUE : FALSE;
        IfxPort_setPinState(g_ledPort, LED_START_PIN + i, on ? IfxPort_State_low : IfxPort_State_high);
    }
}
