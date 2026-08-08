#ifndef __UTILS_H__
#define	__UTILS_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RAM_FUNC
// #define RAM_FUNC __attribute__((section(".time_critical")))

uint32_t HAL_GetTick(void);
void HAL_Delay(uint32_t t);

#ifdef __cplusplus
}
#endif

#endif	// __UTILS_H__
