   
#include "./usart/bsp_usart.h"
#include "sys_tick.h"
#include "at_parser.h"
#include "at_producer.h"

#define AT_CMDTABLE_MAXSIZE   16

typedef struct
{	
	uint16_t  period;      //指令执行周期
	uint8_t   cnt;         //指令生存次数
	Pfunc     proc;        //指令处理函数
	dfunc     hook;  	   //执行错误处理
	
}at_cmd_t;

volatile uint8_t at_cmd_idx;
volatile uint8_t at_cmd_num;
volatile uint8_t at_cmd_en;

uint8_t at_err_cnt;

at_cmd_t cmd_table[AT_CMDTABLE_MAXSIZE];

/*======================================================================*/
int cmd_register(uint16_t period , uint8_t cnt , Pfunc  proc, dfunc hook)
{
	if(at_cmd_num <= AT_CMDTABLE_MAXSIZE)
	{
		cmd_table[at_cmd_num].cnt 		= cnt;
		cmd_table[at_cmd_num].period	= period;
		cmd_table[at_cmd_num].hook 		= hook;
		cmd_table[at_cmd_num].proc      = proc; 
		at_cmd_num++;
		
		return (at_cmd_num);
	}
	else
	{
		AT_LOG("\r\n>>>>over the max size of table\r\n");
		return (0);
	}	
}

void at_cmd_start(uint8_t idx)
{
	if(idx>=1 && idx<= AT_CMDTABLE_MAXSIZE)
	{
		if(cmd_table[idx-1].proc)
		{
			at_cmd_idx = (idx-1);
			at_cmd_en = 1;
			at_err_cnt = cmd_table[at_cmd_idx].cnt;		
			return;			
		}
		AT_LOG("\r\n>>>>not register this cmd \r\n");
		return;	
	}
	AT_LOG("\r\n>>>>over the max size of table\r\n");
}


/*================================================
 at_cmd producer process
=================================================*/
void at_cmd_table_proc(void)
{
	static uint32_t start_time;
	
	
	if(at_cmd_en)	
	{
		if(!start_time){
			start_time = Uptime_Ms();
		}
		if((Uptime_Ms() - start_time) > ((at_err_cnt == cmd_table[at_cmd_idx].cnt) ? 3000: cmd_table[at_cmd_idx].period))
		{
			start_time = 0;
			
			/*send cmd*/
			if(cmd_table[at_cmd_idx].proc())
			{
				if(at_err_cnt)
				{
					at_err_cnt--;         
				}
				if(!at_err_cnt)
				{								
					at_cmd_en = 0;
					if(cmd_table[at_cmd_idx].hook){
						cmd_table[at_cmd_idx].hook(0);  //回处理函数
					}						
				}
				return;
			}	
			at_cmd_en = 0;				
			if(cmd_table[at_cmd_idx].hook){
				cmd_table[at_cmd_idx].hook(1);  //回调处理函数
			}					
		}
	}
	else
	{
		at_err_cnt = 0;
		start_time = 0;
	}
}


