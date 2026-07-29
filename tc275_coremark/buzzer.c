#include "buzzer.h"


/* Complementary output on ATOM2_CH5 (adjacent to CH4, pin not used) */
static const IfxGtm_Atom_ToutMapP buzzerCoutxPin = &IfxGtm_ATOM2_5_TOUT35_P33_13_OUT;

void Buzzer_init(Buzzer_Driver *buzzer, IfxGtm_Atom_ToutMap *pin)
{
    Ifx_GTM *gtm = &MODULE_GTM;

    IfxGtm_enable(gtm);
    buzzer->cmuFrequency = IfxGtm_Cmu_getModuleFrequency(gtm);
    IfxGtm_Cmu_setGclkFrequency(gtm, buzzer->cmuFrequency);
    IfxGtm_Cmu_setClkFrequency(gtm, IfxGtm_Cmu_Clk_0, buzzer->cmuFrequency);
    IfxGtm_Cmu_enableClocks(gtm, IFXGTM_CMU_CLKEN_CLK0);

    /* Init timer on ATOM2_CH0 as time base */
    IfxGtm_Atom_Timer_Config timerConfig;
    IfxGtm_Atom_Timer_initConfig(&timerConfig, gtm);
    timerConfig.base.frequency     = buzzer->cmuFrequency;
    timerConfig.base.isrPriority   = 0;
    timerConfig.base.trigger.enabled  = FALSE;
    timerConfig.atom                = IfxGtm_Atom_2;
    timerConfig.timerChannel        = IfxGtm_Atom_Ch_0;
    timerConfig.clock               = IfxGtm_Cmu_Clk_0;
    timerConfig.triggerOut          = NULL_PTR;
    timerConfig.initPins            = FALSE;
    IfxGtm_Atom_Timer_init(&buzzer->timer, &timerConfig);
    IfxGtm_Atom_Timer_stdIfTimerInit(&buzzer->timerIf, &buzzer->timer);

    /* Timer must be running before PwmHl init */
    IfxStdIf_Timer_run(&buzzer->timerIf);

    /* Init PwmHl: 1 channel, ccx=buzzer pin, coutx=unused */
    IfxGtm_Atom_ToutMapP ccxArray[1]   = { pin };
    IfxGtm_Atom_ToutMapP coutxArray[1] = { buzzerCoutxPin };

    IfxGtm_Atom_PwmHl_Config pwmHlConfig;
    IfxGtm_Atom_PwmHl_initConfig(&pwmHlConfig);
    pwmHlConfig.base.channelCount    = 1;
    pwmHlConfig.base.deadtime        = 0;
    pwmHlConfig.base.minPulse        = 0;
    pwmHlConfig.base.ccxActiveState  = Ifx_ActiveState_high;
    pwmHlConfig.base.coutxActiveState= Ifx_ActiveState_high;
    pwmHlConfig.base.outputMode      = IfxPort_OutputMode_pushPull;
    pwmHlConfig.base.outputDriver    = IfxPort_PadDriver_cmosAutomotiveSpeed1;
    pwmHlConfig.atom                 = IfxGtm_Atom_2;
    pwmHlConfig.timer                = &buzzer->timer;
    pwmHlConfig.ccx                  = ccxArray;
    pwmHlConfig.coutx                = coutxArray;
    pwmHlConfig.initPins             = TRUE;
    IfxGtm_Atom_PwmHl_init(&buzzer->pwmHl, &pwmHlConfig);
    IfxGtm_Atom_PwmHl_stdIfPwmHlInit(&buzzer->pwmHlIf, &buzzer->pwmHl);

    IfxStdIf_PwmHl_setMode(&buzzer->pwmHlIf, Ifx_Pwm_Mode_leftAligned);

    Buzzer_stop(buzzer);
}

void Buzzer_setTone(Buzzer_Driver *buzzer, uint32 frequencyHz, uint8 volumePercent)
{
    if (frequencyHz == 0 || volumePercent == 0)
    {
        Buzzer_stop(buzzer);
        return;
    }

    uint32 periodTicks = (uint32)(buzzer->cmuFrequency / (float32)frequencyHz);
    uint32 onTime      = (periodTicks * volumePercent) / 200;

    Ifx_TimerValue tOn[1] = { onTime };

    IfxStdIf_Timer_disableUpdate(&buzzer->timerIf);
    IfxStdIf_Timer_setPeriod(&buzzer->timerIf, periodTicks);
    IfxStdIf_PwmHl_setOnTime(&buzzer->pwmHlIf, tOn);
    IfxStdIf_Timer_applyUpdate(&buzzer->timerIf);
}

void Buzzer_stop(Buzzer_Driver *buzzer)
{
    Ifx_TimerValue tOn[1] = { 0 };

    IfxStdIf_Timer_disableUpdate(&buzzer->timerIf);
    IfxStdIf_PwmHl_setOnTime(&buzzer->pwmHlIf, tOn);
    IfxStdIf_Timer_applyUpdate(&buzzer->timerIf);
}
