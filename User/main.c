/**
  ******************************************************************************
  * @file    main.c
  * @author  fire
  * @version V1.0
  * @date    2015-xx-xx
  * @brief   通用定时器定时
  ******************************************************************************
  * @attention
  *
  * 实验平台:野火  STM32 F429 开发板
  * 论坛    :http://www.firebbs.cn
  * 淘宝    :https://fire-stm32.taobao.com
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


void ec20_process(void);
void ec20_init(void);

int main(void) 
{
	SysTick_init();
	LED_GPIO_Config();
	TIMx_Configuration();
	usart_init();

    ec20_init();
	
	while(1)
	{        		
		ec20_process();	
	}	
}



/*********************************************END OF FILE**********************/

