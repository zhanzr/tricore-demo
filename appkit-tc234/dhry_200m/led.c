/*
 * led.c - TC234 LEDs on P13.0 ... P13.3 (low active)
 */

#include "led.h"

/* Function to initialize GPIO pins for LEDs */
void initLEDs (void)
{
    /* Initialize GPIO pins for LEDs */
    IfxPort_setPinMode(LED1, IfxPort_Mode_outputPushPullGeneral);
    IfxPort_setPinMode(LED2, IfxPort_Mode_outputPushPullGeneral);
    IfxPort_setPinMode(LED3, IfxPort_Mode_outputPushPullGeneral);
    IfxPort_setPinMode(LED4, IfxPort_Mode_outputPushPullGeneral);

    /* Turn off all LEDs (low active) */
    IfxPort_setPinState(LED1, IfxPort_State_high);
    IfxPort_setPinState(LED2, IfxPort_State_high);
    IfxPort_setPinState(LED3, IfxPort_State_high);
    IfxPort_setPinState(LED4, IfxPort_State_high);
}

void LED1_On(void) {
    IfxPort_setPinState(LED1, IfxPort_State_low);
}

void LED2_On(void) {
    IfxPort_setPinState(LED2, IfxPort_State_low);
}

void LED3_On(void) {
    IfxPort_setPinState(LED3, IfxPort_State_low);
}

void LED4_On(void) {
    IfxPort_setPinState(LED4, IfxPort_State_low);
}

void LED1_Off(void) {
    IfxPort_setPinState(LED1, IfxPort_State_high);
}

void LED2_Off(void) {
    IfxPort_setPinState(LED2, IfxPort_State_high);
}

void LED3_Off(void) {
    IfxPort_setPinState(LED3, IfxPort_State_high);
}

void LED4_Off(void) {
    IfxPort_setPinState(LED4, IfxPort_State_high);
}

void LED1_Toggle(void) {
    IfxPort_setPinState(LED1, IfxPort_State_toggled);
}

void LED2_Toggle(void) {
    IfxPort_setPinState(LED2, IfxPort_State_toggled);
}

void LED3_Toggle(void) {
    IfxPort_setPinState(LED3, IfxPort_State_toggled);
}

void LED4_Toggle(void) {
    IfxPort_setPinState(LED4, IfxPort_State_toggled);
}
