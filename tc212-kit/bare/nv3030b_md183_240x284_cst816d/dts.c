#include "dts.h"
#include "IfxDts_Dts.h"

static boolean g_dtsInitialized = FALSE;

void start_dts_measure(void)
{
    if (!g_dtsInitialized)
    {
        IfxDts_Dts_Config dtsConfig;
        IfxDts_Dts_initModuleConfig(&dtsConfig);

        dtsConfig.isrPriority = 0;   /* polling mode */

        IfxDts_Dts_initModule(&dtsConfig);
        g_dtsInitialized = TRUE;
    }

    IfxDts_Dts_startSensor();
}

float read_dts_celsius(void)
{
    if (!g_dtsInitialized)
    {
        start_dts_measure();
    }

    while (IfxDts_Dts_isBusy())
    {
        __nop();
    }

    return IfxDts_Dts_getTemperatureCelsius();
}
