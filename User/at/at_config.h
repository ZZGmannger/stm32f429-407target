#ifndef AT_CONFIG_H
#define AT_CONFIG_H

#include "stm32f4xx.h"
#include "./usart/bsp_usart.h"
#include "sys_tick.h"

/*at parser config*/
#define AT_USART        (USART2)
#define AT_LOG   		(printf)
#define reg_usart0      (1)
#define reg_usart1      (2)


#define URC_MAX_NUM     (8)
#define URC_MAX_CMDLEN  (36)

#define AT_RESPOND_MAX  (256)

/*at producer config*/
#define AT_CMDTABLE_MAXSIZE     (36)
#define NEXT_CMD_INTERVAL_TIME  (500)

#define my_malloc	(malloc)
#define my_free     (free)


#define list_entry(pnode, type, member)  \
			         ((type *)((char*)pnode - (unsigned long)(&((type*)0)->member)))
						 
struct node
{
	struct node * prev;
	struct node * next;	
};
typedef struct node list_t,*plist;


static __inline void list_init(plist l)
{
	l->next = l->prev = l;
}
static __inline void list_insert_before(plist l, plist n)
{
	n->next = l->next;
	n->prev = l;
	
	l->next->prev = n;	
	l->next = n;
	
}
static __inline void list_insert_after(plist l ,plist n)
{
	n->next = l;
	n->prev = l->prev;
	
	l->prev->next = n;
	l->prev = n;
}

#endif


