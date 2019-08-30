   
#include "./usart/bsp_usart.h"
#include "sys_tick.h"
#include "at_parser.h"


#define AT_LOG   (printf)
#define AT_USART (USART2)

#define URC_MAX_NUM   (8)
#define AT_RESPOND_MAX 128

struct resp_t
{
	uint8_t state;
	uint8_t line_count;
	uint8_t data[AT_RESPOND_MAX];
};
struct resp_t  respond;


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

static void respond_data_process(uint8_t *buffer,uint16_t len)
{		
	char * pend = NULL;	
	char *pdata = (char*)buffer;
	char *sdata = (char*)respond.data;
	
	uint8_t length = 0;
	
	if(len >= AT_RESPOND_MAX)
	{
		AT_LOG("\r\n>>>>>the respond data size over the max size %d\r\n",AT_RESPOND_MAX);
		return;
	}	
	
	if(!respond.state)
	{
		respond.state = 1;		
		
		while(pdata < (char*)(buffer+len))
		{
			/*find \r\n and return it addr*/
			pend = strstr((char *)pdata,"\r\n");
			respond.line_count++;
			
			if(pend == NULL)
			{
				respond.state = 0;	
				AT_LOG("\r\n>>>>>respond error,without end\r\n");
				return;	
			}
									
			*pend = '\0';
			length = strlen(pdata);
			if(length)
			{
				memcpy(sdata , pdata ,length);
			}
			
			sdata += length;
			
			(*sdata) = '\0';
			sdata += 1;
			length += 2;		
			pdata += length;
		}	
	}	
	
}

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
/*等待响应*/
static int wait_reply(uint16_t timeout)
{
	static uint32_t start_time;
	
	while(!respond.state)
	{	   
		if(!start_time){
			start_time = Uptime_Ms();
		}
		if((Uptime_Ms()-start_time) >= timeout)
		{
			start_time = 0;			
			return -1;
		}
	}	
	start_time = 0;
	return 0;
}
/*清除响应数据*/
static void clean_reply(void)
{
	memset(respond.data,0,AT_RESPOND_MAX);
	respond.line_count = 0;
	respond.state = 0;
}

/*有响应发送*/
int8_t at_send_with_reply(const char* cmd , uint16_t timeout)
{
	const char *p_end = cmd;
	clean_reply();	
	
	usart_write(AT_USART,(uint8_t *)cmd , strlen(cmd)); 
	
	if(strstr(p_end,"\r\n") == NULL)
	{
		usart_write(AT_USART,(uint8_t *)"\r\n" , 2); 
	}
	if(wait_reply(timeout))
	{
		AT_LOG("\r\n>>>>>%s:reply timeout\r\n",cmd);
		return -1;	
	}
	return 0;
}
/*无响应发送*/
void at_send_without_reply(const char* cmd)
{
	const char *p_end = cmd;
	usart_write(AT_USART,(uint8_t *)cmd , strlen(cmd)); 
	
	if(strstr(p_end,"\r\n") == NULL)
	{
		usart_write(AT_USART,(uint8_t *)"\r\n" , 2); 
	}
}
/*获取指定行号的响应数据*/
static uint8_t *at_resp_get_line(uint8_t resp_line)
{
	uint8_t idx = 0;
	uint8_t *pdata = NULL;
    if ((resp_line > respond.line_count) || (resp_line <= 0))
    {
        return 0;
    }
	pdata = respond.data;
	
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
	 uint8_t len = 0;
	 uint8_t *resp_line_buf = 0;

     if(at_send_with_reply(cmd,timeout))
	 {
		return (-1);
	 }
	
	 if ((resp_line_buf = at_resp_get_line(line)) == 0)
	 {
		AT_LOG("\r\n>>>>>get the line buffer failed\r\n");
		return -1;
	 }
	 
	 /*diff*/
	 if(memcmp(reply , resp_line_buf , strlen(reply)))
	 {
		AT_LOG("\r\n>>>>>reply is not expected\r\n");
		return (-1);
	 }
	 return 0;
}
int8_t at_send_get_repy(const char*cmd,
						const char* reply_head,
						char *reply,
						uint8_t line,
						uint16_t timeout)
{
	 uint8_t * data = NULL;
	 uint8_t *resp_line_buf = 0;
	 
	 if(at_send_with_reply(cmd,timeout))
	 {
		return (-1);
	 }
	 
	if ((resp_line_buf = at_resp_get_line(line)) == 0)
	{
		AT_LOG("\r\n>>>>>get the line buffer failed\r\n");
		return -1;
	}

	 /*diff*/
	 if(memcmp(reply_head , resp_line_buf , strlen(reply_head)))
	 {
		AT_LOG("\r\n>>>>>the head of reply is not expected\r\n"); 
		return (-1);
	 }
	 data = resp_line_buf + strlen(reply_head);
	 memcpy(reply,data,strlen((char*)data));
	 return 0;
}
