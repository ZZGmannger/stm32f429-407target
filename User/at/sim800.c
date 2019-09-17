#include "sim800.h"
#include "at_parser.h"
#include "at_producer.h"


uint8_t tcp_connected;
uint8_t sim800_power_state;

uint8_t gps_ok;
uint8_t gps_inited;

enum
{
	POWER_OFF,
	POWER_ON
};

/*================================================
sim800 init
=================================================*/
void gps_init(void);
void tcp_init(void);
void urc_init(void);
void sim800_cmd_hook(uint8_t cmd_handle,uint8_t state);

extern uint8_t SAPBR_CONTYPE_handle;
extern uint8_t AT_handle;
extern uint8_t CIPSTART_handle;

void sim800_init(void)
{
	at_parser_init();
	producer_hook_register(sim800_cmd_hook);
	
	tcp_init();
	gps_init();
	urc_init();
	
	/*open the module power*/
//	gpio_bit_write(GPIOA, GPIO_PIN_15, 1);
//	delay_1ms(1000);
//	gpio_bit_write(GPIOA, GPIO_PIN_15, 0);
	
	sim800_power_state = POWER_ON;
	
	at_cmd_start(AT_handle);
}

void gps_cmd_hook(uint8_t cmd_handle,uint8_t state);
void tcp_cmd_hook(uint8_t cmd_handle,uint8_t state);

void sim800_cmd_hook(uint8_t cmd_handle,uint8_t state)
{	
	tcp_cmd_hook(cmd_handle,state);
    gps_cmd_hook(cmd_handle,state);	
}

void sim800_power_deal(void)
{
	static uint32_t time;
	
	if(sim800_power_state == POWER_OFF)
	{
		if(!time)
		{
			//gpio_bit_write(GPIOA, GPIO_PIN_15, 1);
			time = Uptime_Ms();
		}
		if((Uptime_Ms()-time)>1000)
		{
			time = 0;
			//gpio_bit_write(GPIOA, GPIO_PIN_15, 0);
			sim800_power_state = POWER_ON;
		}	
	}
}
/*=========================================================================
URC Callback process
==========================================================================*/
void urc_disconnected_deal(uint8_t *buffer,uint8_t len)
{
	tcp_connected = 0;
	at_cmd_start(CIPSTART_handle);
}
void urc_recieve(uint8_t *buffer,uint8_t len)
{
	printf("\r\ntcp client rec:\r\n");
	for(uint8_t i=0;i<len;i++)
	{
		printf("%c",buffer[i]);
	}
	printf("\r\n");
}
void urc_power_down(uint8_t *buffer,uint8_t len)
{	
	sim800_power_state = POWER_OFF;
}
void urc_init(void)
{
	at_register_urc("CLOSE", urc_disconnected_deal);
	at_register_urc("Rec:", urc_recieve);
	at_register_urc("NORMAL POWER DOWN", urc_power_down);
}
/*=========================================================================
GPRS TCP/IP Command Function
==========================================================================*/
uint8_t AT_handle;
uint8_t CPIN_handle;
uint8_t CREG_handle;
uint8_t CGATT_handle;
uint8_t CIPSTART_handle;

int AT_START_CMD(void);
int AT_CPIN_CMD(void);
int AT_CREG_CMD(void);
int AT_CGATT_CMD(void);
int AT_CIPSTART_CMD(void);

void tcp_init(void)
{
	AT_handle		= cmd_register(2000 , 2 , AT_START_CMD);
	CPIN_handle		= cmd_register(5000 , 3 , AT_CPIN_CMD);
	CREG_handle		= cmd_register(5000 , 3 , AT_CREG_CMD);
	CGATT_handle	= cmd_register(5000 , 3 , AT_CGATT_CMD);
	
	CIPSTART_handle = cmd_register(10000 , 3 , AT_CIPSTART_CMD);	
}
int tcp_send_data(uint8_t *buff , uint8_t len)
{
	static uint32_t time;
	char start[2]={0x1a};
	if(!time)
	{
		time = Uptime_Ms();
	}
	if((Uptime_Ms() - time) > 5000)
	{
		time = 0;
		if(tcp_connected && gps_ok)
		{		
			char data[50]={0};
			if(at_send_get_reply("AT+CGNSINF","+CGNSINF: ",data,2,1000))
			{
				return (-1);
			}
			
			at_send_cmd("AT+CIPSEND");
			//delay_1ms(5);
			at_send_data(data);
			at_send_cmd(start);
		}
	}
	return 0;
}


void tcp_cmd_hook(uint8_t cmd_handle,uint8_t state)
{
	if(cmd_handle == AT_handle)
	{
		if(state == 1){
			at_cmd_start(CPIN_handle);	
		}
		else{
			sim800_power_state =POWER_OFF;
			at_cmd_start(AT_handle);	
		}
	}
	else if(cmd_handle == CPIN_handle)
	{
		if(state == 1){
			at_cmd_start(CREG_handle);	
		}
		else{
			at_cmd_start(AT_handle);	
		}
	}
	else if(cmd_handle == CREG_handle)
	{
		if(state == 1){
			at_cmd_start(CGATT_handle);	
		}
		else{
			at_cmd_start(AT_handle);	
		}
	}
	else if(cmd_handle == CGATT_handle)
	{
		if(state == 1){
			at_cmd_start(CIPSTART_handle);	
		}
		else{
			at_cmd_start(AT_handle);	
		}
	}
	else if(cmd_handle == CIPSTART_handle)
	{
		if(state == 1){
			tcp_connected = 1;
			if(!gps_ok)
			at_cmd_start(SAPBR_CONTYPE_handle);
		}
		else{
			at_cmd_start(CGATT_handle);	
		}
	}
}

int AT_START_CMD(void)
{
	if(at_send_cmp_reply("AT","OK",2,300))
	{
		return CMD_ERR;
	}
	
	return CMD_OK;
}
	
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

int AT_CGATT_CMD(void)
{
	char ret[32]={0};
	if(at_send_get_reply("AT+CGATT?","+CGATT:",ret,2,2000))
	{
		return CMD_ERR;
	}	
	if(strstr(ret,"1") == NULL)
	{
		return CMD_ERR;
	}
	return CMD_OK;
}


int AT_CIPSTART_CMD(void)
{
	char ret[32]={0};
	if(at_send_get_reply("AT+CIPSTART=\"TCP\",\"47.106.221.173\",\"443\"",NULL,ret,4,8000))
	{
		return CMD_ERR;
	}	
	if(strstr(ret,"CONNECT") == NULL)
	{
		if(strstr(ret,"ALREADY CONNECT") == NULL)
		{
			return CMD_ERR;
		}
	}
	return CMD_OK;
}






















/*=========================================================================
GPS Command Function
==========================================================================*/
uint8_t SAPBR_CONTYPE_handle;
uint8_t SAPBR_APN_handle;
uint8_t SAPBR_ACTIVATE_handle;
uint8_t SAPBR_GET_handle;

uint8_t CNTPCID_handle;
uint8_t CNTP_SET_handle;
uint8_t CNTP_GET_handle;
uint8_t CNTP_SYN_handle;

uint8_t CCLK_handle;
uint8_t CGNSSAV_handle;

uint8_t HTTPINIT_handle;
uint8_t HTTPPARA_handle;
uint8_t HTTPURL_handle;
uint8_t HTTPACTION_handle;

uint8_t HTTPTERM_handle;
uint8_t CGNSCHK_handle;

uint8_t CGNSPWR_handle;
uint8_t CGNSAID_handle;
uint8_t CGNSINF_handle;


int AT_SAPBR_CONTYPE_CMD(void);
int AT_SAPBR_APN_CMD(void);
int AT_SAPBR_ACTIVATE_CMD(void);
int AT_SAPBR_GET_CMD(void);

int AT_CNTPCID_CMD(void);
int AT_CNTP_SET_CMD(void);
int AT_CNTP_GET_CMD(void);
int AT_CNTP_SYN_CMD(void);

int AT_CCLK_CMD(void);
int AT_CCLK_CMD(void);

int AT_HTTPINIT_CMD(void);
int AT_HTTPPARA_CMD(void);
int AT_HTTPACTION_CMD(void);
int AT_HTTPTERM_CMD(void);
int AT_HTTPURL_CMD(void);

int AT_CGNSCHK_CMD(void);
int AT_CGNSPWR_CMD(void);
int AT_CGNSAID_CMD(void);
int AT_CGNSINF_CMD(void);

void gps_init(void)
{
	SAPBR_CONTYPE_handle	= cmd_register(3000 , 2 , AT_SAPBR_CONTYPE_CMD);
	SAPBR_APN_handle		= cmd_register(3000 , 2 , AT_SAPBR_APN_CMD);
	SAPBR_ACTIVATE_handle	= cmd_register(8000 , 2 , AT_SAPBR_ACTIVATE_CMD);
	SAPBR_GET_handle		= cmd_register(3000 , 2 , AT_SAPBR_GET_CMD);

	CNTPCID_handle			= cmd_register(3000 , 2 , AT_CNTPCID_CMD);
	CNTP_SET_handle			= cmd_register(3000 , 2 , AT_CNTP_SET_CMD);
	CNTP_GET_handle			= cmd_register(3000 , 2 , AT_CNTP_GET_CMD);
	CNTP_SYN_handle			= cmd_register(8000 , 2 , AT_CNTP_SYN_CMD);

	CCLK_handle				= cmd_register(3000 , 2 , AT_CCLK_CMD);
	CGNSSAV_handle			= cmd_register(3000 , 2 , AT_CCLK_CMD);

	HTTPINIT_handle			= cmd_register(3000 , 2 , AT_HTTPINIT_CMD);
	HTTPPARA_handle			= cmd_register(3000 , 2 , AT_HTTPPARA_CMD);
	HTTPURL_handle			= cmd_register(3000 , 2 , AT_HTTPURL_CMD);
	HTTPACTION_handle		= cmd_register(25000, 2 , AT_HTTPACTION_CMD);
	HTTPTERM_handle			= cmd_register(3000 , 2 , AT_HTTPTERM_CMD);
	
	CGNSCHK_handle			= cmd_register(3000 , 2 , AT_CGNSCHK_CMD);
	CGNSPWR_handle			= cmd_register(8000 , 2 , AT_CGNSPWR_CMD);
	CGNSAID_handle			= cmd_register(15000, 2 , AT_CGNSAID_CMD);
	CGNSINF_handle			= cmd_register(3000 , 100 , AT_CGNSINF_CMD);	
}

void gps_cmd_hook(uint8_t cmd_handle,uint8_t state)
{
	if(cmd_handle == SAPBR_CONTYPE_handle)
	{
		if(state == 1){
			at_cmd_start(SAPBR_APN_handle);
		}
		else{
			at_cmd_start(SAPBR_CONTYPE_handle);
		}
	}
	else if(cmd_handle == SAPBR_APN_handle)
	{
		if(state == 1){
			at_cmd_start(SAPBR_ACTIVATE_handle);
		}
		else{
			/**/
		}
	}
	else if(cmd_handle == SAPBR_ACTIVATE_handle)
	{
		if(state == 1){
			at_cmd_start(SAPBR_GET_handle);
		}
		else{
			/**/
		}
	}
	else if(cmd_handle == SAPBR_GET_handle)
	{
		if(state == 1){
			at_cmd_start(CNTPCID_handle);
		}
		else{
			/**/
		}
	}
	else if(cmd_handle == CNTPCID_handle)
	{
		if(state == 1){
			at_cmd_start(CNTP_SET_handle);
		}
		else{
			/**/
		}
	}
	else if(cmd_handle == CNTP_SET_handle)
	{
		if(state == 1){
			at_cmd_start(CNTP_GET_handle);
		}
		else{
			/**/
		}
	}
	else if(cmd_handle == CNTP_GET_handle)
	{
		if(state == 1){
			at_cmd_start(CNTP_SYN_handle);
		}
		else{
			/**/
		}
	}
	else if(cmd_handle == CNTP_SYN_handle)
	{
		if(state == 1){
			at_cmd_start(CCLK_handle);
		}
		else{
			/**/
		}
	}
	else if(cmd_handle == CCLK_handle)
	{
		if(state == 1){
			at_cmd_start(CGNSSAV_handle);
		}
		else{
			/**/
		}
	}
	else if(cmd_handle == CGNSSAV_handle)
	{
		if(state == 1){
			at_cmd_start(HTTPINIT_handle);
		}
		else{
			/**/
		}
	}
	else if(cmd_handle == HTTPINIT_handle)
	{
		if(state == 1){
			at_cmd_start(HTTPPARA_handle);
		}
		else{
			/**/
		}
	}
	else if(cmd_handle == HTTPPARA_handle)
	{
		if(state == 1){
			at_cmd_start(HTTPURL_handle);
		}
		else{
			/**/
		}
	}
	else if(cmd_handle == HTTPURL_handle)
	{
		if(state == 1){
			at_cmd_start(HTTPACTION_handle);
		}
		else{
			/**/
		}
	}
	else if(cmd_handle == HTTPACTION_handle)
	{
		if(state == 1){
			at_cmd_start(HTTPTERM_handle);
		}
		else{
			/**/
		}
	}
	else if(cmd_handle == HTTPTERM_handle)
	{
		if(state == 1){
			at_cmd_start(CGNSCHK_handle);
		}
		else{
			/**/
		}
	}
	else if(cmd_handle == CGNSCHK_handle)
	{
		if(state == 1){
			at_cmd_start(CGNSPWR_handle);
		}
		else{
			/**/
		}
	}
	else if(cmd_handle == CGNSPWR_handle)
	{
		if(state == 1){
			at_cmd_start(CGNSAID_handle);
		}
		else{
			/**/
		}
	}
	else if(cmd_handle == CGNSAID_handle)
	{
		if(state == 1){
			gps_ok = 1;
		}
		else{
			////
		}
	}
	else if(cmd_handle == CGNSINF_handle)
	{
		at_cmd_start(CGNSINF_handle);
	}
}

int AT_SAPBR_CONTYPE_CMD(void)
{
	if(at_send_cmp_reply("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"","OK",2,2000))
	{
		return CMD_ERR;
	}

	return CMD_OK;
}
  
int AT_SAPBR_APN_CMD(void)
{
	if(at_send_cmp_reply("AT+SAPBR=3,1,\"APN\",\"3GNET\"","OK",2,2000))
	{
		return CMD_ERR;
	}

	return CMD_OK;
}  
 
int AT_SAPBR_ACTIVATE_CMD(void)
{
	if(at_send_cmp_reply("AT+SAPBR=1,1","OK",3,6000))
	{
		return CMD_ERR;
	}

	return CMD_OK;
}  

int AT_SAPBR_GET_CMD(void)
{
    char ret[20]={0};
	if(at_send_get_reply("AT+SAPBR=2,1","+SAPBR: ",ret,2,2000))
	{
		return CMD_ERR;
	}
	return CMD_OK;
}   
  
int AT_CNTPCID_CMD(void)
{
	if(at_send_cmp_reply("AT+CNTPCID=1","OK",2,2000))
	{
		return CMD_ERR;
	}
	return CMD_OK;
}  

int AT_CNTP_SET_CMD(void)
{
	if(at_send_cmp_reply("AT+CNTP=\"202.112.29.82\"","OK",2,2000))
	{
		return CMD_ERR;
	}
	return CMD_OK;
} 

int AT_CNTP_GET_CMD(void)
{
	char ret[20]={0};
	if(at_send_get_reply("AT+CNTP?","+CNTP: ",ret,2,2000))
	{
		return CMD_ERR;
	}
	return CMD_OK;
} 

int AT_CNTP_SYN_CMD(void)
{
	if(at_send_cmp_reply("AT+CNTP","+CNTP: ",4,5000))
	{
		return CMD_ERR;
	}
	return CMD_OK;
}    
   
int AT_CCLK_CMD(void)
{
	char ret[20]={0};
	if(at_send_get_reply("AT+CCLK?","+CCLK: ",ret,2,2000))
	{
		return CMD_ERR;
	}
	return CMD_OK;
} 

int AT_CGNSSAV_CMD(void)
{
	if(at_send_cmp_reply("AT+CGNSSAV=3,3","OK",2,2000))
	{
		return CMD_ERR;
	}
	return CMD_OK;
}

int AT_HTTPINIT_CMD(void)
{
	if(at_send_cmp_reply("AT+HTTPINIT","OK",2,2000))
	{
		return CMD_ERR;
	}
	return CMD_OK;
}  

int AT_HTTPPARA_CMD(void)
{
	if(at_send_cmp_reply("AT+HTTPPARA=\"CID\",1","OK",2,2000))
	{
		return CMD_ERR;
	}
	return CMD_OK;
}    
   
int AT_HTTPURL_CMD(void)
{
	if(at_send_cmp_reply("AT+HTTPPARA=\"URL\",\"http://wepodownload.mediatek.com/EPO_GPS_3_1.DAT\"","OK",2,2000))
	{
		return CMD_ERR;
	}
	return CMD_OK;
}  

int AT_HTTPACTION_CMD(void)
{
    char ret[20]={0};
	if(at_send_get_reply("AT+HTTPACTION=0","+HTTPACTION: ",ret,3,20000))
	{
		return CMD_ERR;
	}
	return CMD_OK;
}    
   
int AT_HTTPTERM_CMD(void)
{
	if(at_send_cmp_reply("AT+HTTPTERM","OK",2,2000))
	{
		return CMD_ERR;
	}
	return CMD_OK;
} 

int AT_CGNSCHK_CMD(void)
{
	char ret[20]={0};
	if(at_send_get_reply("AT+CGNSCHK=3,1","+CGNSCHK: ",ret,2,3000))
	{
		return CMD_ERR;
	}
	return CMD_OK;
} 

int AT_CGNSPWR_CMD(void)
{
	if(at_send_cmp_reply("AT+CGNSPWR=1","+CGNSPWR: 1",4,6000))
	{
		return CMD_ERR;
	}
	return CMD_OK;
}    

int AT_CGNSAID_CMD(void)
{
	if(at_send_cmp_reply("AT+CGNSAID=31,1,1","+CGNSAID: OK",4,12000))
	{
		return CMD_ERR;
	}
	return CMD_OK;
}  

int AT_CGNSINF_CMD(void)
{
	if(at_send_cmp_reply("AT+CGNSINF","+CGNSINF:",2,2000))
	{
		return CMD_ERR;
	}
	return CMD_OK;
} 


