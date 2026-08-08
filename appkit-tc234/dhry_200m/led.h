/*
 * led.h - TC234 LEDs on P13.0 ... P13.3 (low active)
 */

#ifndef LED_H_
#define LED_H_

#include <stdint.h>
#include <inttypes.h>

#include "Ifx_Types.h"
#include "IfxPort.h"

/* LEDs */
#define LED1                        &MODULE_P13,0
#define LED2                        &MODULE_P13,1
#define LED3                        &MODULE_P13,2
#define LED4                        &MODULE_P13,3

void initLEDs (void);

void LED1_On(void);
void LED2_On(void);
void LED3_On(void);
void LED4_On(void);

void LED1_Off(void);
void LED2_Off(void);
void LED3_Off(void);
void LED4_Off(void);

void LED1_Toggle(void);
void LED2_Toggle(void);
void LED3_Toggle(void);
void LED4_Toggle(void);

#endif /* LED_H_ */
