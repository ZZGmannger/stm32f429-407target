#ifndef __AT_PARSER_H
#define __AT_PARSER_H

#include "stm32f4xx.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

typedef void (*urc_func)(uint8_t *buf , uint8_t len);

void at_parser_init(void);
uint8_t at_register_urc(const char *cmd , urc_func callback);
void at_unregister_urc(const char * cmd);

uint8_t mem_cmp(const void* a,const void* b, uint8_t len);

void at_send_without_reply(const char* cmd);
int8_t at_send_cmp_reply(const char* cmd , const char *reply , uint8_t line ,uint16_t timeout);
int8_t at_send_get_repy(const char*cmd ,const char* reply_head,char *reply , uint8_t line , uint16_t timeout);

#endif


