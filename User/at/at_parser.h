#ifndef __AT_PARSER_H
#define __AT_PARSER_H

#include "at_config.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#define CMD_ERR   (-1)
#define CMD_OK    (0)

typedef void (*urc_func)(uint8_t *buf , uint8_t len);

void at_parser_init(void);
int8_t at_register_urc(const char *cmd , urc_func callback);

void at_send_cmd(const char* cmd);
void at_send_data(const char*buf);
int8_t at_send_cmp_reply(const char* cmd , const char *reply , uint8_t line ,uint16_t timeout);
int8_t at_send_get_reply(const char*cmd ,const char* reply_head,char *reply , uint8_t line , uint16_t timeout);

#endif


