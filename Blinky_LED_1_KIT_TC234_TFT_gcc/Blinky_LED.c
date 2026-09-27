/**********************************************************************************************************************
 * \file Blinky_LED.c
 * \copyright Copyright (C) Infineon Technologies AG 2019
 *
 * GCC/CLI port of the official Blinky_LED_1_KIT_TC234_TFT example,
 * extended to blink ALL FOUR user LEDs (D107-D110 on P13.0 ... P13.3,
 * low active). Timing uses the official STM-based wait mechanism.
 *********************************************************************************************************************/

/*********************************************************************************************************************/
/*-----------------------------------------------------Includes------------------------------------------------------*/
/*********************************************************************************************************************/
#include "IfxPort.h"
#include "Bsp.h"
#include "Blinky_LED.h"

/*********************************************************************************************************************/
/*------------------------------------------------------Macros-------------------------------------------------------*/
/*********************************************************************************************************************/
#define LED_PORT        &MODULE_P13                                         /* LED port: P13 (D107-D110)            */
#define LED_COUNT       4                                                   /* P13.0 ... P13.3                      */
#define WAIT_TIME       250                                                 /* Wait time per step (milliseconds)    */

/*********************************************************************************************************************/
/*---------------------------------------------Function Implementations----------------------------------------------*/
/*********************************************************************************************************************/
/* This function initializes the port pins which drive the 4 user LEDs */
void initLED(void)
{
    uint8 i;

    for (i = 0; i < LED_COUNT; i++)
    {
        /* Initialization of the LEDs used in this example */
        IfxPort_setPinModeOutput(LED_PORT, i, IfxPort_OutputMode_pushPull, IfxPort_OutputIdx_general);

        /* Switch OFF the LED (low-level active) */
        IfxPort_setPinHigh(LED_PORT, i);
    }
}

/* This function shifts a rotating pattern across the 4 LEDs, 250 milliseconds per step */
void blinkLED(void)
{
    static uint8 n = 0;                                                         /* current LED index                    */
    uint8 i;

    for (i = 0; i < LED_COUNT; i++)
    {
        if (i == n)
        {
            IfxPort_setPinLow(LED_PORT, i);                                     /* ON  (low active)                     */
        }
        else
        {
            IfxPort_setPinHigh(LED_PORT, i);                                    /* OFF                                  */
        }
    }
    n = (uint8)((n + 1u) % LED_COUNT);

    waitTime(IfxStm_getTicksFromMilliseconds(BSP_DEFAULT_TIMER, WAIT_TIME));    /* Wait 250 milliseconds               */
}
