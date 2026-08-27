#ifndef DTS_H
#define DTS_H

#include <stdint.h>
#include <inttypes.h>
#include "Ifx_Types.h"

// Initialize and start a temperature measurement run
void start_dts_measure(void);

// Read current die temperature in Celsius
float read_dts_celsius(void);

#endif /* DTS_H */

