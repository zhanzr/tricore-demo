#ifndef BUZZER_H
#define BUZZER_H

#include "Ifx_Types.h"

/* Passive buzzer on P33.0 driven by GTM TOM0_CH4 (TOUT22), 2048 Hz PWM. */
#define BUZZER_FREQ_HZ  2048u

void Buzzer_init(void);
void Buzzer_setDutyCycle(uint8 percent); /* 0..100 */
void Buzzer_stop(void);

#endif /* BUZZER_H */
