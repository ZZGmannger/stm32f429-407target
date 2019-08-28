#include "./EC20/ec20.h"
#include "at_parser.h"


uint8_t module_state;
enum
{
	INIT,
	CONFIG,
	CONNECTED
};

/*URC»Øµ÷º¯Êý============================================>*/
void urc_disconnected_deal(uint8_t *buffer,uint8_t len)
{
	module_state = INIT;
}
void urc_start_recieve(uint8_t *buffer,uint8_t len)
{
	at_send_without_reply("AT+QIRD=0,32"); 
}
void urc_server_recieve(uint8_t *buffer,uint8_t len)
{
	
	if(!memcmp("+QIRD:",buffer,5))
	{
		/*data*/
	}
	
}
/*========================================================*/

 
void ec20_init(void)
{
	at_parser_init();
	
	at_register_urc("+QIURC: \"recv\"", urc_start_recieve);
	at_register_urc("+QIRD:", urc_server_recieve);
	at_register_urc("+QIURC: \"closed\"", urc_disconnected_deal);
}



void ec20_process(void)
{
	char ret[32]={0};
	switch(module_state)
	{
		case INIT: 
		{
			/*1.AT*/			
			if(at_send_cmp_reply("AT","OK",2,3000))
			{
				return;
			}; 
			/*2.AT+CPIN?*/
			if(at_send_get_repy("AT+CPIN?","+CPIN:",ret,2,3000))
			{
				return;  //if not ok for 20s the reboot the module
			}
			if(memcmp("READY",ret,5))
			{
				return;
			}
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
		}break;
		case CONFIG: 
		{
			//Configure a Context
			//AT+QICSGP=1,1,"UNINET","","",1
			//OK
			if(at_send_cmp_reply("AT+QICSGP=1,1,\"UNINET\",\"\",\"\",1","OK",2,3000))
			{
				return;  
			}			
			//Activate a Context
			//AT+QIACT=1
			//OK
			if(at_send_cmp_reply("AT+QIACT=1","OK",2,3000))
			{
				return;  
			}	
			//AT+QIOPEN=1,0,"TCP","220.180.239.212",8009,0,0
			//+QIOPEN: 0,0
			if(at_send_get_repy("AT+QIOPEN=1,0,\"TCP\",\"47.106.221.173\",443,0,0","+QIOPEN:",ret,2,1000))
			{
				return;  
			}
			if(ret[2]=='0' && ret[2]=='0')	
			{
				module_state = CONNECTED;
			}				
		}break;
		case CONNECTED:
		{
			//AT+QISTATE=1,0
		}break;			
	}
}




void reboot_module(void)
{

}

