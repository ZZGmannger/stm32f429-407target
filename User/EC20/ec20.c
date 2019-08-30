#include "./EC20/ec20.h"
#include "at_parser.h"
#include "sys_tick.h"

uint8_t module_state;
enum
{
	IDLE,
	INIT,
	CONFIG,
	CONNECTED
};



/*URC回调函数,运行在串口中断中============================================>*/
void urc_disconnected_deal(uint8_t *buffer,uint8_t len)
{
	module_state = INIT;
}

void urc_start_recieve(uint8_t *buffer,uint8_t len)
{
	uint8_t fifo[64]={0};
	uint8_t num = 0;
	char *pdata = (char*)buffer;	
	char *pend = NULL;
	
	uint8_t channel = 0;
	uint8_t buf_len = 0;
	
	channel = (*(pdata+1))-0x30;
	
	pdata = pdata +3;
	
	pend = strstr(pdata,"\r\n");
	
	*pend = '\0'; 
	
	buf_len =  atoi(pdata);
	
	pdata +=  (strlen(pdata) + 2);
	
	for(uint8_t i=0;i<buf_len;i++)
	{
		fifo[i] = pdata[i];
	}
	printf("\r\n>>>>>>tcp recieve ok\r\n");
		
}

/*========================================================*/

 
void ec20_init(void)
{
	at_parser_init();
	
	at_register_urc("+QIURC: \"recv\"", urc_start_recieve);
	at_register_urc("+QIURC: \"closed\"", urc_disconnected_deal);
}



void ec20_process(void)
{
	char ret[32]={0};
	switch(module_state)
	{
		case IDLE:
		{
			if(at_send_cmp_reply("AT+QISTATE?","OK",2,3000))
			{
				if(at_send_cmp_reply("AT+QISTATE?","+QISTATE:",2,3000))
				{
					module_state = CONNECTED;
				}
			}
			module_state = INIT;
		}break;
		case INIT: 
		{			
			/*1.AT*/			
			if(at_send_cmp_reply("AT","OK",2,3000))
			{
				return;
			}; 
			delay_ms(3000);
			/*2.AT+CPIN?*/
			if(at_send_get_repy("AT+CPIN?","+CPIN:",ret,2,3000))
			{
				return;  //if not ok for 20s the reboot the module
			}
			if(memcmp("READY",ret,5))
			{
				return;
			}
			delay_ms(3000);
			/*3.AT+CREG?*/
			if(at_send_get_repy("AT+CREG?","+CREG:",ret,2,3000))
			{
				return; 
			}
			if(ret[2]>='1')
			{
				module_state = CONFIG;
			}
			return;  //if the ret keep 0 for 90s the reboot the module
			delay_ms(3000);
		}break;
		case CONFIG: 
		{
			if(at_send_get_repy("AT+QIOPEN=1,0,\"TCP\",\"47.106.221.173\",443,0,0","+QIOPEN:",ret,4,5000))
			{
				return;  
			}
			
			if(at_send_cmp_reply("AT+QISTATE?","OK",2,3000))
			{
				if(at_send_cmp_reply("AT+QISTATE?","+QISTATE:",2,3000))
				{
					module_state = CONNECTED;
				}
			}
				
			delay_ms(3000);
		}break;
		case CONNECTED:
		{
			if(at_send_cmp_reply("AT+QISTATE?","+QISTATE:",2,3000))
			{
				module_state = INIT;
			}
		
			delay_ms(3000);
		}break;			
	}
}




void reboot_module(void)
{

}

