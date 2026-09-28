/*
 * led.h - TC212 LEDs: P02.0..P02.5, P11.10, P11.11 (low active)
 */

#ifndef LED_H_
#define LED_H_

#include <stdint.h>
#include <inttypes.h>

#include "Ifx_Types.h"
#include "IfxPort.h"

/* LEDs */
#define LED1                        &MODULE_P02,0
#define LED2                        &MODULE_P02,1
#define LED3                        &MODULE_P02,2
#define LED4                        &MODULE_P02,3
#define LED5                        &MODULE_P02,4
#define LED6                        &MODULE_P02,5
#define LED7                        &MODULE_P11,10
#define LED8                        &MODULE_P11,11

#define LED_COUNT                   8

void initLEDs(void);
void setLED(uint8 index, boolean on);
void toggleLED(uint8 index);

#endif /* LED_H_ */
