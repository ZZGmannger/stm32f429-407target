/**
  ******************************************************************************
  * @file    main.c
  * @author  fire
  * @version V1.0
  * @date    2015-xx-xx
  * @brief   ͨ�ö�ʱ����ʱ
  ******************************************************************************
  * @attention
  *
  * ʵ��ƽ̨:Ұ��  STM32 F429 ������
  * ��̳    :http://www.firebbs.cn
  * �Ա�    :https://fire-stm32.taobao.com
  *
  ******************************************************************************
  */
#include "sys_tick.h"
#include "stm32f4xx.h"
#include "./tim/bsp_general_tim.h"
#include "./led/bsp_led.h"
#include "./usart/bsp_usart.h"
#include "ff.h"

#include "at_parser.h"

uint8_t aad[20];
void ec20_process(void);
int main(void) 
{
	SysTick_init();
	LED_GPIO_Config();
	TIMx_Configuration();
	usart_init();

    at_parser_init();
	
	while(1)
	{        		
		ec20_process();				
	}	
}



/*********************************************END OF FILE**********************/

