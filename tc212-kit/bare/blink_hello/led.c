/*
 * led.c - TC212 LEDs: P02.0..P02.5, P11.10, P11.11 (low active)
 */

#include "led.h"

static const Ifx_P *g_ledPorts[LED_COUNT] = {
    &MODULE_P02, &MODULE_P02, &MODULE_P02, &MODULE_P02,
    &MODULE_P02, &MODULE_P02, &MODULE_P11, &MODULE_P11
};
static const uint8 g_ledPins[LED_COUNT] = { 0, 1, 2, 3, 4, 5, 10, 11 };

void initLEDs(void)
{
    uint8 i;
    for (i = 0; i < LED_COUNT; i++)
    {
        IfxPort_setPinMode(g_ledPorts[i], g_ledPins[i], IfxPort_Mode_outputPushPullGeneral);
        IfxPort_setPinState(g_ledPorts[i], g_ledPins[i], IfxPort_State_high);  /* off (low active) */
    }
}

void setLED(uint8 index, boolean on)
{
    if (index >= LED_COUNT)
    {
        return;
    }
    IfxPort_setPinState(g_ledPorts[index], g_ledPins[index], on ? IfxPort_State_low : IfxPort_State_high);
}

void toggleLED(uint8 index)
{
    if (index >= LED_COUNT)
    {
        return;
    }
    IfxPort_setPinState(g_ledPorts[index], g_ledPins[index], IfxPort_State_toggled);
}
