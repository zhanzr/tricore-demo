#ifndef BUZZER_H
#define BUZZER_H

#include "Gtm/Atom/Timer/IfxGtm_Atom_Timer.h"
#include "Gtm/Atom/PwmHl/IfxGtm_Atom_PwmHl.h"

typedef struct
{
    IfxGtm_Atom_Timer       timer;        /**< GTM ATOM timer driver handle */
    IfxGtm_Atom_PwmHl       pwmHl;        /**< GTM ATOM PWM HL driver handle */
    IfxStdIf_Timer          timerIf;      /**< Timer standard interface */
    IfxStdIf_PwmHl          pwmHlIf;      /**< PwmHl standard interface */
    float32                 cmuFrequency; /**< GTM CMU clock frequency in Hz */
} Buzzer_Driver;

void Buzzer_init(Buzzer_Driver *buzzer, IfxGtm_Atom_ToutMap *pin);
void Buzzer_setTone(Buzzer_Driver *buzzer, uint32 frequencyHz, uint8 volumePercent);
void Buzzer_stop(Buzzer_Driver *buzzer);

#endif /* BUZZER_H */
