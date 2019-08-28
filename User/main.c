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
#include "./usart/bsp_debug_usart.h"
#include "ff.h"
/**
  * @brief  主函数
  * @param  无
  * @retval 无
  */
 /**
  ******************************************************************************
  *                              定义变量
  ******************************************************************************
  */
FATFS fs;													/* FatFs文件系统对象 */
FIL fnew;													/* 文件对象 */
FRESULT res_sd;                /* 文件操作结果 */
UINT fnum;            					  /* 文件成功读写数量 */
BYTE ReadBuffer[1024]={0};        /* 读缓冲区 */
BYTE WriteBuffer[] =              /* 写缓冲区*/
"欢迎使用野火STM32 F429开发板 今天是个好日子，新建文件系统测试文件\r\n";  


uint8_t cnt;
void sd_mount(void)
{
	res_sd = f_mount(&fs,"0:",1);

	if(res_sd == FR_NO_FILESYSTEM)
	{
		printf("》SD卡还没有文件系统，即将进行格式化...\r\n");
		/* 格式化 */
		res_sd=f_mkfs("0:",0,0);							
		
		if(res_sd == FR_OK)
		{
			printf("》SD卡已成功格式化文件系统。\r\n");
			/* 格式化后，先取消挂载 */
			res_sd = f_mount(NULL,"0:",1);			
			/* 重新挂载	*/			
			res_sd = f_mount(&fs,"0:",1);
		}
		else
		{
			printf("《《格式化失败。》》\r\n");
			while(1);
		}
	}
	else if(res_sd!=FR_OK)
	{
		printf("！！SD卡挂载文件系统失败。(%d)\r\n",res_sd);
		printf("！！可能原因：SD卡初始化不成功。\r\n");
		while(1);
	}
	else
	{
		printf("》文件系统挂载成功，可以进行读写测试\r\n");
	}
}

void sd_dismount(void)
{
	f_mount(NULL,"0:",1);
}



int main(void) 
{
	SysTick_init();
	LED_GPIO_Config();
	TIMx_Configuration();
	usart_init();
	
	sd_mount();
		
	res_sd = f_open(&fnew, "log.txt",FA_CREATE_ALWAYS | FA_WRITE );
	if (res_sd != FR_OK )
	{
		printf("！！打开/创建文件失败。\r\n");
	}
	while(1)
	{        
		
		usart_write(USART1 , (uint8_t*)"this is a test\r\n", 16);
		delay_ms(1000);
		
		
		if(cnt>100)
		{
			f_close(&fnew);
			sd_dismount();
			printf("file done\r\n");
		}
		else
		{
			res_sd = f_write(&fnew,WriteBuffer,sizeof(WriteBuffer),&fnum);
			if(res_sd!=FR_OK)
			{
				printf("！！文件写入失败：(%d)\n",res_sd);
			}    
			printf("cnt=%d\r\n",cnt);
		}
		cnt++;
	}
}



/*********************************************END OF FILE**********************/

