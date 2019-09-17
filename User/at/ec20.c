#include "ec20.h"
#include "at_parser.h"
#include "at_producer.h"


uint8_t AT_handle;
uint8_t CPIN_handle;
uint8_t CREG_handle;

uint8_t QISTATE_handle;
uint8_t QIOPEN_handle;
uint8_t QICLOSE_handle;

uint8_t tcp_conected;

/*================================================
CMD_1: AT (check whether the connect is ok) 
=================================================*/
int AT_START(void)
{
	if(at_send_cmp_reply("AT","OK",2,1000))
	{
		return CMD_ERR;
	}
	
	return CMD_OK;
}

/*=======================================================
CMD_2: AT+CPIN? (check whether  identify (U)SIM card) 
      
Explain: If failed to identify (U)SIM card in 20s, then 
	     reboot the module.
=========================================================*/
int AT_CPIN_CMD(void)
{
	char ret[32]={0};
	if(at_send_get_reply("AT+CPIN?","+CPIN:",ret,2,2000))
	{
		return CMD_ERR;
	}	
	if(strstr(ret,"READY") == NULL)
	{
		return CMD_ERR;
	}
	return CMD_OK;
}

/*=============================================================
CMD_3: AT+CREG? (check whether   register on CS domain service) 
      
Explain: If failed to register on CS domain service in 90s, 
		 then reboot the module
==============================================================*/
int AT_CREG_CMD(void)
{
	char ret[32]={0};
	if(at_send_get_reply("AT+CREG?","+CREG:",ret,2,2000))
	{
		return CMD_ERR;
	}	
	if(atoi(ret+3)% 2 == 0)
	{
		return CMD_ERR;
	}
	return CMD_OK;
}
/*=============================================================
CMD_4: AT+QISTATE (check whether   register on CS domain service) 
      
Explain: If failed to register on CS domain service in 90s, 
		 then reboot the module
==============================================================*/
int AT_QISTATE_CMD(void)
{
	if(at_send_cmp_reply("AT+QISTATE?","OK",2,2000))
	{
		return CMD_ERR;
	}		
	return CMD_OK;
}
/*=============================================================
CMD_4: AT+QIOPEN (connect to the server) 
      
Explain: If failed to register on CS domain service in 90s, 
		 then reboot the module
==============================================================*/
int AT_QIOPEN_CMD(void)
{
	if(at_send_cmp_reply("AT+QIOPEN=1,0,\"TCP\",\"47.106.221.173\",443,0,1","OK",2,2000))
	{
		return CMD_ERR;
	}		
	return CMD_OK;
}

/*=============================================================
CMD_4: AT+QICLOSE (connect to the server) 
      
Explain: If failed to register on CS domain service in 90s, 
		 then reboot the module
==============================================================*/
int AT_QICLOSE_CMD(void)
{
	if(at_send_cmp_reply("AT+QICLOSE=0","OK",2,5000))
	{
		return CMD_ERR;
	}		
	return CMD_OK;
}

void hex_to_str(uint8_t*pbDest, uint8_t*pbSrc, int nLen)
{
	char ddl,ddh;
	int i;

	for (i=0; i<nLen; i++)
	{
		ddh = 48 + pbSrc[i] / 16;
		ddl = 48 + pbSrc[i] % 16;
		if (ddh > 57) ddh = ddh + 7;
		if (ddl > 57) ddl = ddl + 7;
		pbDest[i*2] = ddh;
		pbDest[i*2+1] = ddl;
	}
	pbDest[nLen*2] = '\0';
}

void AT_QISEND(uint8_t *buffer , uint8_t len)
{
	char data[48]={0};
	static uint8_t error;
	if(tcp_conected)
	{
			strcpy(data,"AT+QISENDEX=0,\"aa0a0122334455667788bb\"");
		
			if(at_send_cmp_reply(data,"SEND OK",2,2000))
			{
				error++;
				if(error>3)
				{
					at_cmd_start(QISTATE_handle);
					tcp_conected = 0;
					error = 0;					
				}
				return;
			}	
			error = 0;			
	}
}


/*=====================*/
void tcp_init(void)
{
	AT_handle   	= cmd_register(3000 , 5, AT_START);
	CPIN_handle 	= cmd_register(3000 , 2 , AT_CPIN_CMD);
	CREG_handle 	= cmd_register(3000, 9 , AT_CREG_CMD);
	
	QISTATE_handle  = cmd_register(3000,  1 , AT_QISTATE_CMD);
	QIOPEN_handle 	= cmd_register(3000,  1 , AT_QIOPEN_CMD);
	QICLOSE_handle  = cmd_register(3000,  1 , AT_QICLOSE_CMD);
	
    at_cmd_start(AT_handle);
}


void check_tcp_state(uint8_t *buffer,uint8_t len)
{
	if(atoi((char*)(buffer+2)) == 0)
	{
		tcp_conected = 1;
	}
	else
	{
		at_cmd_start(QICLOSE_handle);
	}
}

/*URC回调函数,运行在串口中断中============================================>*/
void urc_disconnected_deal(uint8_t *buffer,uint8_t len)
{
	at_cmd_start(QICLOSE_handle);
	tcp_conected = 0;
}
uint8_t fifo[1024]={0};
void urc_start_recieve(uint8_t *buffer,uint8_t len)
{
	
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
	printf("rec: %s",fifo);
		
}

/*========================================================*/

void ec20_cmd_hook(uint8_t handle ,uint8_t state)
{

}

void ec20_init(void)
{
	at_parser_init();
	producer_hook_register(ec20_cmd_hook);
	tcp_init();
	at_register_urc("+QIURC: \"recv\"", urc_start_recieve);
	at_register_urc("+QIURC: \"closed\"", urc_disconnected_deal);
	at_register_urc("+QIOPEN: ", check_tcp_state);
}


