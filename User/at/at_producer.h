#ifndef __AT_PRODUCER_H
#define __AT_PRODUCER_H

#include "at_config.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

typedef int (*Pfunc)(void);
typedef void (*HookFunc)(uint8_t id , uint8_t state);

void producer_hook_register(HookFunc hook);
int  cmd_register(uint16_t period , uint8_t cnt , Pfunc  proc);

void at_cmd_start(uint8_t idx);
void at_cmd_scheduler(void);

#endif


