   
#include "./usart/bsp_usart.h"
#include "sys_tick.h"
#include "at_parser.h"


#define AT_LOG   (printf)
#define AT_USART (USART2)


uint8_t mem_cmp(const void *a,const void *b, uint8_t len)
{
	 char * ca = NULL;
	 char * cb = NULL;	
	
	ca = (char *)a;
	cb = (char *)b;
	
	for(uint8_t i=0;i<len;i++)
	{
		if(ca[i] == cb[i])
		{
			return (-1);
		}
	}
	return 0;
}


struct res
{
	uint8_t state;
	uint8_t line_count;
	uint8_t data[3][32];
};
struct res  respond;


typedef struct 
{
	const char * cmd;
	urc_func func;
}urc_t;	


#define URC_MAX_NUM   (4)

urc_t URC_table[URC_MAX_NUM];

uint8_t at_register_urc(const char *cmd , urc_func callback)
{
	for(uint8_t i=0;i<URC_MAX_NUM;i++)
	{
		if((URC_table[i].cmd == NULL) && URC_table[i].func == NULL)
		{
			URC_table[i].cmd = cmd;
			URC_table[i].func  = callback;
			return 0;
		}
	}	
	AT_LOG("create urc:%s command failed\r\n",cmd);
	return (-1);
}
void at_unregister_urc(const char * cmd)
{
	for(uint8_t i=0;i<URC_MAX_NUM;i++)
	{
		if(!memcmp(cmd , URC_table[i].cmd , strlen(cmd)))
		{
			URC_table[i].cmd = NULL;
			URC_table[i].func = NULL;
			return;
		}
	}
	AT_LOG("%s :command not found in URC_table\r\n",cmd);
}


 
static uint8_t URC_data_process(uint8_t *buffer,uint16_t len)
{
	for(uint8_t i=0;i<URC_MAX_NUM;i++)
	{
		if(!memcmp(URC_table[i].cmd , buffer , strlen(URC_table[i].cmd)))
		{
			URC_table[i].func(buffer , len);
			return 1;
		}
	}
	return 0;
}

static void respond_data_process(uint8_t *buffer,uint16_t len)
{	
	uint8_t j=0;
	
	respond.state =1;
	for(uint8_t i=0;i<len;i++)
	{		
		if((buffer[i]=='\r')&&(i<len-1)&&(buffer[i+1]=='\n'))
		{
			/*not the last \r\n*/
			if(i !=(len-2))
			{		
				respond.line_count++;
			}
			i++;
			j=0;			
		}
		else
		{
			if(buffer[i] != ' ')
			{
				respond.data[respond.line_count][j++] = buffer[i];
			}
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
   usart_register(2, uart_callback);
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
	for(uint8_t i=0;i<respond.line_count;i++)
	{
		for(uint8_t j=0;j<32;j++)
		{
			respond.data[i][j] = 0;
		}
	}
	respond.line_count = 0;
	respond.state = 0;
}

/*有响应发送*/
int8_t at_send_with_reply(const char* cmd , uint16_t timeout)
{
	clean_reply();	
	
	usart_write(AT_USART,(uint8_t *)cmd , strlen(cmd)); 
	if(cmd[strlen(cmd)-1] != '\n')
	{
		usart_write(AT_USART,(uint8_t *)"\r\n" , 2); 
	}
	
	if(wait_reply(timeout))
	{
		AT_LOG("%s:reply timeout\r\n",cmd);
		return -1;	
	}
	return 0;
}
void at_send_without_reply(const char* cmd)
{
	usart_write(AT_USART,(uint8_t *)cmd , strlen(cmd)); 
	if(cmd[strlen(cmd)-1] != '\n')
	{
		usart_write(AT_USART,(uint8_t *)"\r\n" , 2); 
	}
}
/*获取指定行号的响应数据*/
static uint8_t *at_resp_get_line(uint8_t resp_line)
{
    if ((resp_line > respond.line_count+1) || (resp_line <= 0))
    {
        return 0;
    }
	
	return respond.data[resp_line-1];
}
static int8_t at_resp_parse_line_args(uint8_t resp_line, const char *resp_expr, ...)
{
    va_list args;
    int resp_args_num = 0;
    uint8_t *resp_line_buf = 0;

    if ((resp_line_buf = at_resp_get_line(resp_line)) == 0)
    {
		AT_LOG("get the line buffer failed\r\n");
        return -1;
    }

    va_start(args, resp_expr);

    resp_args_num = vsscanf((const char *)resp_line_buf, resp_expr, args);

    va_end(args);
   
    return resp_args_num;
}

int8_t at_send_cmp_reply(const char* cmd,
						 const char *reply,
						 uint8_t line,
						 uint16_t timeout)
{
	 uint8_t ret[32]={0};
	 if(at_send_with_reply(cmd,timeout))
	 {
		return (-1);
	 }
	 at_resp_parse_line_args(line,"%s",ret);
	 /*diff*/
	 if(memcmp(reply,ret,strlen(reply)))
	 {
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
	 char ret[32]={0};
	 char * data = NULL;
	 if(at_send_with_reply(cmd,timeout))
	 {
		return (-1);
	 }
	 at_resp_parse_line_args(line,"%s",ret);
	 /*diff*/
	 if(memcmp(reply_head,ret,strlen(reply_head)))
	 {
		return (-1);
	 }
	 data = ret+strlen(reply_head);
	 memcpy(reply,data,strlen(data));
	 return 0;
}
