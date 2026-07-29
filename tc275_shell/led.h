/*
 * led.h
 *
 *  Created on: Jul 29, 2026
 *      Author: user1
 */

#ifndef LED_H_
#define LED_H_

#include <stdint.h>
#include <inttypes.h>

#include "ASCLIN_Shell_UART.h"

#include "Ifx_Types.h"
#include "IfxPort.h"
#include "IfxCbs_reg.h"

/* LEDs */
#define LED1                        &MODULE_P33,8
#define LED2                        &MODULE_P33,9
#define LED3                        &MODULE_P33,10
#define LED4                        &MODULE_P33,11

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
