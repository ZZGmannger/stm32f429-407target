#ifndef __SYS_TICK_H
#define __SYS_TICK_H

#include "stm32f4xx.h"

void delay_ms(__IO u32 nTime);
void SysTick_init(void);
void timing_delay_decrement(void);
uint32_t Uptime_Ms(void);

#endif

