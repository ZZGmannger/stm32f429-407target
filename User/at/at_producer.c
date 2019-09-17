#include "at_parser.h"
#include "at_producer.h"

enum
{
	NORMAL,
	CRASH
};

typedef struct
{	
	uint16_t  period;      //指令执行周期
	uint8_t   cnt;         //指令生存次数
	Pfunc     proc;        //指令处理函数	
}at_cmd_t;


typedef struct  
{
	list_t list;
	uint8_t data;	
}fifo_t;

void fifo_delete_first(fifo_t *l)
{	
	fifo_t * pf = NULL;
	if(l->list.next !=  NULL)
	{
            pf = list_entry(l->list.next , fifo_t,list);
            
            l->list.next->next->prev = &l->list;
            l->list.next = l->list.next->next;
            
            my_free(pf);

	}	
}

typedef struct
{
	uint8_t   cmd_num;
	fifo_t    fifo;
	at_cmd_t  cmd_table[AT_CMDTABLE_MAXSIZE];
	HookFunc  complete_hook;
	
}at_producer_t;

at_producer_t   producer;


void producer_hook_register(HookFunc hook)
{
	producer.complete_hook = hook;
	list_init(&producer.fifo.list);
}

int cmd_register(uint16_t period , uint8_t cnt , Pfunc  proc)
{
	if(producer.cmd_num <= AT_CMDTABLE_MAXSIZE)
	{
		producer.cmd_table[producer.cmd_num].cnt 	 = cnt;
		producer.cmd_table[producer.cmd_num].period	 = period;
		producer.cmd_table[producer.cmd_num].proc    = proc; 
		producer.cmd_num++;
		
		return (producer.cmd_num);
	}
	else
	{
		AT_LOG("\r\n>>>>over the max size of table\r\n");
		return (0);
	}	
}

void at_cmd_start(uint8_t idx)
{
	uint8_t type = 0;
	/*1.check if idx is register*/
	if(idx>=1 && idx <= AT_CMDTABLE_MAXSIZE)
	{
		if(producer.cmd_table[idx-1].proc)
		{
			/*2.add to fifo*/		
			fifo_t* pnode = my_malloc(sizeof(fifo_t));
			if(pnode == NULL)
			{
				AT_LOG("at cmd start malloc fail\r\n");
				return;
			}
			
			pnode->data = idx;
			if(type == NORMAL){
				list_insert_after(&producer.fifo.list, &pnode->list);
			}
			else{
				list_insert_before(&producer.fifo.list, &pnode->list);
			}
			return;	
		}
		AT_LOG("\r\n>>>>not register this cmd \r\n");
		return;	
	}
	AT_LOG("\r\n>>>>over the max size of table\r\n");
}
uint8_t get_ready_cmd(void)
{
	uint8_t idx = 0;
	fifo_t * p = NULL;
	if(producer.fifo.list.next != &producer.fifo.list)
	{
		p = list_entry(producer.fifo.list.next ,fifo_t ,list);
		idx = p->data;
		fifo_delete_first(&producer.fifo);	
		
		return (idx);
	}	
	return 0;
}

/*================================================
 at_cmd producer process
=================================================*/
void at_cmd_scheduler(void)
{
	static uint32_t start_time;
	static uint8_t current_handle;
	static uint8_t error_remain;
	static uint16_t interval_time = 0;
	
	if(!current_handle)
	{
		current_handle = get_ready_cmd();
		if(current_handle == 0)
		{
			return;
		}
		
		error_remain = producer.cmd_table[current_handle-1].cnt;
		start_time = 0;
		interval_time = NEXT_CMD_INTERVAL_TIME;
	}
	else
	{		
		if(!start_time){
			start_time = Uptime_Ms();	
		}
		if((Uptime_Ms() - start_time) > interval_time)
		{
			start_time = 0;	
			interval_time = producer.cmd_table[current_handle-1].period;
		
			/*send cmd*/
			if(producer.cmd_table[current_handle-1].proc())
			{
				if(error_remain)
				{
					error_remain--;         
				}
				if(!error_remain)
				{								
					if(producer.complete_hook){
						producer.complete_hook(current_handle , 0);  //回处理函数
					}
					current_handle = 0;					
				}
				return;
			}					
			if(producer.complete_hook){
				producer.complete_hook(current_handle, 1);  //回处理函数
			}
			current_handle = 0;	
		}
	}
}


