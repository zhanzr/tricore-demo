#ifndef __CUSTOM_DEF_H__
#define __CUSTOM_DEF_H__

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils.h"
#include "ASCLIN_Shell_UART.h"

#define	configTICK_RATE_HZ	1000

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#ifndef COMPILER_NAME
#if defined(__TASKING__)
    #if defined(__CTC__)
        #define COMPILER_NAME "TASKING TriCore " __VERSION__
    #else
        #define COMPILER_NAME "TASKING " __VERSION__
    #endif
#elif defined(__ARMCC_VERSION)
    #define COMPILER_NAME "ARMClang " TOSTRING(__ARMCC_VERSION)
#elif defined(__GNUC__) && defined(__ARM_ARCH)
    #define COMPILER_NAME "GCC ARM " __VERSION__
#elif defined(__GNUC__)
    #define COMPILER_NAME "GCC " __VERSION__
#else
    #define COMPILER_NAME "Unknown Compiler"
#endif
#endif


#endif
