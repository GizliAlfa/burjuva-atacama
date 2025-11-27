#ifndef __SYSTEM_STM32F10X_H
#define __SYSTEM_STM32F10X_H

#include <stdint.h>

extern uint32_t SystemCoreClock;
extern void SystemInit(void);
extern void SystemCoreClockUpdate(void);

#endif
