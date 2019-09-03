   
#include "./usart/bsp_usart.h"
#include "sys_tick.h"
#include "at_parser.h"



#define AT_USART (USART2)

#define URC_MAX_NUM   (8)
#define AT_RESPOND_MAX 256

struct resp_t
{
	uint8_t rec;
	uint8_t state;
	uint8_t line_cnt;
	char fifo[AT_RESPOND_MAX];
};
struct resp_t  respond;

enum
{
	REPS_IDLE,
	REPS_WAIT,
};

typedef struct 
{
	char cmd[36];
	urc_func func;
}urc_t;	
urc_t URC_table[URC_MAX_NUM];

uint8_t at_register_urc(const char *cmd , urc_func callback)
{
	for(uint8_t i=0;i<URC_MAX_NUM;i++)
	{
		if((URC_table[i].cmd[0] == NULL) && URC_table[i].func == NULL)
		{
			memcpy(URC_table[i].cmd,cmd,strlen(cmd));
			URC_table[i].func  = callback;
			return 0;
		}
	}	
	AT_LOG("create urc:%s command failed\r\n",cmd);
	return (255);
}
void at_unregister_urc(const char * cmd)
{
	for(uint8_t i=0;i<URC_MAX_NUM;i++)
	{
		if(URC_table[i].func != NULL)
		{
			if(!memcmp(cmd , URC_table[i].cmd , strlen(cmd)))
			{
				memset(URC_table[i].cmd,0,sizeof(URC_table[i].cmd));
				URC_table[i].func = NULL;
				return;
			}
		}
	}
	AT_LOG("%s :command not found in URC_table\r\n",cmd);
}


 
static uint8_t URC_data_process(uint8_t *buffer,uint16_t len)
{
	char *pdata = NULL;	

	uint8_t head_len = 0;
	
	for(uint8_t i=0;i<URC_MAX_NUM;i++)
	{
		if(URC_table[i].func != NULL)
		{
			pdata = strstr((char*)buffer , URC_table[i].cmd);
			if(pdata != NULL)
			{				
				head_len = strlen(URC_table[i].cmd);
				URC_table[i].func((uint8_t *)(pdata+head_len) , strlen(pdata)-head_len);
				return 1;
			}
		}
	}
	return 0;
}


void respond_data_process(uint8_t *buffer , uint16_t len);

static void uart_callback(uint8_t *data , uint8_t len)
{
	if(URC_data_process(data,len))
	{
		return;
	}
    respond_data_process(data,len);
}

void at_parser_init(void)
{
	if(AT_USART == USART1)
	{
		usart_register(1, uart_callback);
	}
	else if(AT_USART == USART2)
	{
		usart_register(2, uart_callback);
	}
	else
	{
		AT_LOG("\r\n>>>>>AT parser register fail\r\n");
	}
}


uint8_t enter_to_end(char* from_str , char* to_str)
{
	uint8_t line_cnt = 0;
	uint8_t line_len = 0;
	
	char * idx = NULL;	
	char *pdata = from_str;
	char *sdata = to_str;
	
	uint8_t length = strlen(from_str);
   
   while(pdata < (char*)(from_str + length))
   {
		idx = strstr((char *)pdata,"\r\n");
  
		if(idx == NULL)
		{
			idx = strstr((char *)pdata,"\r");
			if(idx == NULL)
			{
				return line_cnt;
			}
			*idx ='\0';
			line_len = strlen(pdata);
			if(line_len)
			{
				memcpy(sdata , pdata ,line_len);
				sdata += line_len;
				*(sdata++) = '\0';	
			}
			return line_cnt+1;
		};
		line_cnt++;
		
		*idx = '\0';
		line_len = 	strlen(pdata);
		if(line_len)
		{
			memcpy(sdata , pdata ,line_len);
		}
		
		sdata += line_len;
		*(sdata++) = '\0';	
		
		line_len += 2;
		
		pdata += line_len;	
    }
	return line_cnt;
}


void respond_data_process(uint8_t *buffer , uint16_t len)
{	
	uint8_t idx = 0;

	if(respond.state == REPS_WAIT)
	{
		uint8_t cnt = 0;
		for(uint8_t i=0;i < respond.line_cnt;i++)
		{
			idx += strlen(respond.fifo + idx) + 1;
		}
		if((AT_RESPOND_MAX - idx) > len)
		{
			cnt = enter_to_end((char*)buffer , respond.fifo+idx);
			respond.line_cnt += cnt;
			respond.rec = 1;
		}
	}
}
void at_send_cmd(const char* cmd)
{
	usart_write(AT_USART,(uint8_t *)cmd , strlen(cmd)); 

	if(strstr(cmd,"\r\n") == NULL)
	{
		usart_write(AT_USART,(uint8_t *)"\r\n" , 2); 
	}
}

void clean_respond(void)
{
	respond.line_cnt = 0;
	respond.rec = 0;
	respond.state = REPS_IDLE;
	memset(respond.fifo,0,AT_RESPOND_MAX);
}
char *at_resp_get_line(uint8_t resp_line)
{
	uint8_t idx = 0;
	char *pdata = NULL;
    if ((resp_line > respond.line_cnt) || (resp_line <= 0))
    {
		AT_LOG("\r\n>>>>>input resp_line over the exit line\r\n");
        return 0;
    }
	pdata = respond.fifo;
	
	for(uint8_t i=0;(i<resp_line-1);i++)
	{
	    idx = strlen((char*)pdata)+1;
		pdata += idx;
	}	
	return pdata;
}
 
int8_t at_send_cmp_reply(const char* cmd,
						 const char *reply,
						 uint8_t line,
						 uint16_t timeout)
{
	char *resp_line_buf = NULL;	 
	uint32_t time = 0;
	
	/*send cmd*/
	at_send_cmd(cmd);
	respond.state =REPS_WAIT;
	
	/*wait for reply*/
	if(!time)
	{
		time = Uptime_Ms();
	}
	while((Uptime_Ms() - time) < timeout)
	{
		if(respond.rec)
		{
			respond.rec = 0;
			if(line <= respond.line_cnt)
			{	
				resp_line_buf = at_resp_get_line(line);
				
				/*diff*/
				if(memcmp(reply , resp_line_buf , strlen(reply)))
				{
					AT_LOG("\r\n>>>>>reply is not expected\r\n");
					clean_respond();
					return (-1);
				}
				clean_respond();
				return 0;
			}
		}
	}
	AT_LOG("\r\n>>>>>%s:reply timeout\r\n",cmd);
	clean_respond();
	return(-1);		
}

int8_t at_send_get_repy(const char*cmd,
						  const char* reply_head,
						  char *reply,
						  uint8_t line,
						  uint16_t timeout)
{
	uint32_t time = 0;
	char * data = NULL;
	char *resp_line_buf = 0;

	/*send cmd*/
	at_send_cmd(cmd);
	respond.state =REPS_WAIT;
	 
	/*wait for reply*/
	if(!time)
	{
		time = Uptime_Ms();
	}
	while((Uptime_Ms() - time) < timeout)
	{
		if(respond.rec)
		{
			respond.rec = 0;
			if(line <= respond.line_cnt)
			{	
				resp_line_buf = at_resp_get_line(line);

				/*diff*/
				if(memcmp(reply , resp_line_buf , strlen(reply)))
				{
					AT_LOG("\r\n>>>>>reply is not expected\r\n");
					clean_respond();
					return (-1);
				}

				data = resp_line_buf + strlen(reply_head);
				memcpy(reply,data,strlen((char*)data));
				clean_respond();
				return 0;
			}
		}
	}
	AT_LOG("\r\n>>>>>%s:reply timeout\r\n",cmd);
	clean_respond();
	return(-1);		

}


