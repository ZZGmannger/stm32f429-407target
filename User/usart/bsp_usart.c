/**
  ******************************************************************************
  * @file    bsp_debug_usart.c
  * @author  fire
  * @version V1.0
  * @date    2015-xx-xx
  * @brief   重定向c库printf函数到usart端口，中断接收模式
  ******************************************************************************
  * @attention
  *
  * 实验平台:野火  STM32 F429 开发板  
  * 论坛    :http://www.firebbs.cn
  * 淘宝    :https://fire-stm32.taobao.com
  *
  ******************************************************************************
  */ 
  
#include "./usart/bsp_usart.h"
#include "sys_tick.h"

usart_cb_t  usart1_callback = NULL;
usart_cb_t  usart2_callback = NULL;

 /**
  * @brief  配置嵌套向量中断控制器NVIC
  * @param  无
  * @retval 无
  */
static void NVIC_Configuration(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;

	/* 嵌套向量中断控制器组选择 */
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

	/* 配置USART为中断源 */
	NVIC_InitStructure.NVIC_IRQChannel = DEBUG_USART_IRQ;
	/* 抢断优先级为1 */
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
	/* 子优先级为1 */
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
	/* 使能中断 */
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	/* 初始化配置NVIC */
	NVIC_Init(&NVIC_InitStructure);
	
	/* 配置USART为中断源 */
	NVIC_InitStructure.NVIC_IRQChannel = CMD_USART_IRQ;
	/* 抢断优先级为1 */
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	/* 子优先级为1 */
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	/* 使能中断 */
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	/* 初始化配置NVIC */
	NVIC_Init(&NVIC_InitStructure);
}

void usart_register(uint8_t port,usart_cb_t cb)
{
	if(port == 1)
	{
		usart1_callback = cb;
	}
	else if(port == 2)
	{
		usart2_callback = cb;
	}
}

void usart_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
		
	RCC_AHB1PeriphClockCmd(DEBUG_USART_RX_GPIO_CLK|DEBUG_USART_TX_GPIO_CLK,ENABLE);
	RCC_APB2PeriphClockCmd(DEBUG_USART_CLK, ENABLE);

	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;  
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    //tx
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Pin = DEBUG_USART_TX_PIN  ;  
	GPIO_Init(DEBUG_USART_TX_GPIO_PORT, &GPIO_InitStructure);
	//rx
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Pin = DEBUG_USART_RX_PIN;
	GPIO_Init(DEBUG_USART_RX_GPIO_PORT, &GPIO_InitStructure);

	
	GPIO_PinAFConfig(DEBUG_USART_RX_GPIO_PORT,DEBUG_USART_RX_SOURCE,DEBUG_USART_RX_AF);
	GPIO_PinAFConfig(DEBUG_USART_TX_GPIO_PORT,DEBUG_USART_TX_SOURCE,DEBUG_USART_TX_AF);


	USART_InitStructure.USART_BaudRate = DEBUG_USART_BAUDRATE;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

	USART_Init(DEBUG_USART, &USART_InitStructure); 

	USART_ITConfig(DEBUG_USART, USART_IT_RXNE, ENABLE);
	USART_ITConfig(DEBUG_USART, USART_IT_IDLE, ENABLE);

	USART_Cmd(DEBUG_USART, ENABLE);
	
/*********************************************************************/		
	RCC_AHB1PeriphClockCmd( CMD_USART_RX_GPIO_CLK|CMD_USART_TX_GPIO_CLK, ENABLE);
	RCC_APB1PeriphClockCmd(CMD_USART_CLK, ENABLE);

	GPIO_PinAFConfig(CMD_USART_RX_GPIO_PORT,CMD_USART_RX_SOURCE, CMD_USART_RX_AF);
	GPIO_PinAFConfig(CMD_USART_TX_GPIO_PORT,CMD_USART_TX_SOURCE,CMD_USART_TX_AF);

	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	//tx
	GPIO_InitStructure.GPIO_Pin = CMD_USART_TX_PIN  ;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(CMD_USART_TX_GPIO_PORT, &GPIO_InitStructure);
	//rx
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Pin = CMD_USART_RX_PIN;
	GPIO_Init(CMD_USART_RX_GPIO_PORT, &GPIO_InitStructure);
			
	USART_InitStructure.USART_BaudRate = CMD_USART_BAUDRATE;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No ;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(CMD_USART, &USART_InitStructure); 

	NVIC_Configuration();
	USART_ITConfig(CMD_USART, USART_IT_RXNE, ENABLE);
	//USART_ITConfig(CMD_USART, USART_IT_IDLE, ENABLE);

	USART_Cmd(CMD_USART, ENABLE);
}

/*****************  发送一个字符 **********************/
void usart_sendbyte( USART_TypeDef * pUSARTx, uint8_t ch)
{
	/* 发送一个字节数据到USART */
	USART_SendData(pUSARTx,ch);
		
	/* 等待发送数据寄存器为空 */
	while (USART_GetFlagStatus(pUSARTx, USART_FLAG_TXE) == RESET);	
}
void usart_write(USART_TypeDef * pUSARTx , uint8_t *buffer, uint8_t len)
{
	while(len--)
	{
		usart_sendbyte(pUSARTx ,*buffer++);
	}
}

///重定向c库函数printf到串口，重定向后可使用printf函数
int fputc(int ch, FILE *f)
{
		/* 发送一个字节数据到串口 */
		USART_SendData(DEBUG_USART, (uint8_t) ch);
		
		/* 等待发送完毕 */
		while (USART_GetFlagStatus(DEBUG_USART, USART_FLAG_TXE) == RESET);		
	
		return (ch);
}

#define USART_MAX_SIZE   128
void USART1_IRQHandler(void)                
{
	uint16_t clear=0;
	static uint8_t rec_buff[USART_MAX_SIZE];
	static uint8_t rec_len;
	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)  
	{		
		rec_buff[rec_len] = USART_ReceiveData(USART1);
		if(rec_len < USART_MAX_SIZE)
		{
			rec_len++;
		}			
	} 
	if(USART_GetITStatus(USART1, USART_IT_IDLE) != RESET)  
	{
		clear = USART1->DR;
		clear = USART1->SR;
		
		if(usart1_callback != NULL && rec_len > 1)
		{
			usart1_callback(rec_buff,rec_len);
		}
		else
		{
			
		}
		rec_len = 0;
	} 

} 

static uint8_t a_rec_buff[USART_MAX_SIZE];
static uint8_t a_rec_len;
volatile static uint32_t time;

uint8_t rec_flag;

void uart_idle_clean(void)
{
	time = 0;
	rec_flag = 1;
}
void uart_idle_deal(void)
{
	if(rec_flag)
	{
		if(!time){
			time = Uptime_Ms();
		}
		if(Uptime_Ms() - time > 20)
		{
			time = 0;
			rec_flag = 0;
			usart_write(USART1,a_rec_buff,a_rec_len);
			if(usart2_callback != NULL && a_rec_len > 1)
			{
				usart2_callback(a_rec_buff,a_rec_len);
			}
			else
			{
				
			}
			a_rec_len = 0;
		}
	}
}
	

void USART2_IRQHandler(void)                
{
	//uint16_t clear=0;


	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)  
	{		
		a_rec_buff[a_rec_len] = USART_ReceiveData(USART2);
		if(a_rec_len < USART_MAX_SIZE)
		{
			a_rec_len++;
		}	
		uart_idle_clean();		
	} 
//	if(USART_GetITStatus(USART2, USART_IT_IDLE) != RESET)  
//	{
//		clear = USART2->DR;
//		clear = USART2->SR;
//		
//		usart_write(USART1,rec_buff,rec_len);
//		if(usart2_callback != NULL && rec_len > 1)
//		{
//			usart2_callback(rec_buff,rec_len);
//		}
//		else
//		{
//			
//		}
//		rec_len = 0;
//	} 

} 
/*********************************************END OF FILE**********************/
