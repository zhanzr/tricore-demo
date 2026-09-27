#include "buzzer.h"
#include "IfxGtm.h"
#include "IfxGtm_Cmu.h"
#include "IfxGtm_Tom.h"
#include "Gtm/Tom/Pwm/IfxGtm_Tom_Pwm.h"
#include "IfxScuWdt.h"

static IfxGtm_Tom_Pwm_Driver g_buzzerDriver;
static uint32 g_periodTicks;
static Ifx_GTM *g_gtm;

void Buzzer_init(void)
{
    g_gtm = &MODULE_GTM;

    /* Enable GTM */
    IfxGtm_enable(g_gtm);

    /* GCLK = module frequency; enable GCLK and the fixed clocks used by TOM */
    float32 cmuFreq = IfxGtm_Cmu_getModuleFrequency(g_gtm);
    IfxGtm_Cmu_setGclkFrequency(g_gtm, cmuFreq);
    IfxGtm_Cmu_enableClocks(g_gtm, IFXGTM_CMU_CLKEN_FXCLK);

    /* TOM PWM config: TOM0_CH4 -> P33.0 (TOUT22) */
    IfxGtm_Tom_Pwm_Config pwmConfig;
    IfxGtm_Tom_Pwm_initConfig(&pwmConfig, g_gtm);

    pwmConfig.tom             = IfxGtm_Tom_0;
    pwmConfig.tomChannel      = IfxGtm_Tom_Ch_4;
    /* Fxclk1 = GCLK / 16 = 12.5 MHz -> 6103 ticks for 2048 Hz (fits 16-bit
     * TOM CM0/CM1). Compute from the GCLK frequency directly; reading the
     * channel clock register before init returns the reset source, not Fxclk1. */
    pwmConfig.clock           = IfxGtm_Tom_Ch_ClkSrc_cmuFxclk1;
    float32 chanClk = IfxGtm_Cmu_getGclkFrequency(g_gtm) / 16.0f;
    pwmConfig.period          = (uint32)(chanClk / (float32)BUZZER_FREQ_HZ);
    pwmConfig.dutyCycle       = 0;                        /* start silent */
    pwmConfig.signalLevel     = Ifx_ActiveState_high;
    pwmConfig.synchronousUpdateEnabled = FALSE;
    pwmConfig.immediateStartEnabled    = TRUE;
    pwmConfig.pin.outputPin   = &IfxGtm_TOM0_4_TOUT22_P33_0_OUT;
    pwmConfig.pin.outputMode  = IfxPort_OutputMode_pushPull;
    pwmConfig.pin.padDriver   = IfxPort_PadDriver_cmosAutomotiveSpeed1;

    IfxGtm_Tom_Pwm_init(&g_buzzerDriver, &pwmConfig);

    g_periodTicks = pwmConfig.period;
}

void Buzzer_setDutyCycle(uint8 percent)
{
    if (percent > 100)
    {
        percent = 100;
    }

    uint32 onTicks = (g_periodTicks * percent) / 100u;

    /* Write CM1 directly for a synchronous runtime duty update */
    Ifx_GTM_TOM *tom = g_buzzerDriver.tom;
    IfxGtm_Tom_Ch_setCompareOne(tom, g_buzzerDriver.tomChannel, onTicks);
}

void Buzzer_stop(void)
{
    Ifx_GTM_TOM *tom = g_buzzerDriver.tom;
    IfxGtm_Tom_Ch_setCompareOne(tom, g_buzzerDriver.tomChannel, 0);
}
