#ifndef __AT_PRODUCER_H
#define __AT_PRODUCER_H

#include "stm32f4xx.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

typedef int (*Pfunc)(void);
typedef void (*dfunc)(uint8_t parameter);

int cmd_register(uint16_t period , uint8_t cnt , Pfunc  proc, dfunc hook);
void at_cmd_start(uint8_t idx);
void at_cmd_table_proc(void);

#endif


