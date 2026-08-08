#include "dts.h"
#include "IfxDts_Dts.h"

static boolean g_dtsInitialized = FALSE;

void start_dts_measure(void)
{
    if (!g_dtsInitialized)
    {
        IfxDts_Dts_Config dtsConfig;
        
        // Populate default configuration settings
        IfxDts_Dts_initModuleConfig(&dtsConfig);

        // Polling mode: Disable interrupts
        dtsConfig.isrPriority = 0; 

        // Initialize DTS hardware module
        IfxDts_Dts_initModule(&dtsConfig);
        g_dtsInitialized = TRUE;
    }

    // Trigger temperature measurement sensor
    IfxDts_Dts_startSensor();
}

float read_dts_celsius(void)
{
    if (!g_dtsInitialized)
    {
        start_dts_measure();
    }

    // Wait until DTS hardware finishes analog measurement
    while (IfxDts_Dts_isBusy())
    {
        __nop();
    }

    // Return converted temperature value in Celsius
    return IfxDts_Dts_getTemperatureCelsius();
}

