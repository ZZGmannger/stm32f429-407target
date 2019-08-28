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
#include "./usart/bsp_debug_usart.h"
#include "ff.h"
/**
  * @brief  ������
  * @param  ��
  * @retval ��
  */
 /**
  ******************************************************************************
  *                              �������
  ******************************************************************************
  */
FATFS fs;													/* FatFs�ļ�ϵͳ���� */
FIL fnew;													/* �ļ����� */
FRESULT res_sd;                /* �ļ�������� */
UINT fnum;            					  /* �ļ��ɹ���д���� */
BYTE ReadBuffer[1024]={0};        /* �������� */
BYTE WriteBuffer[] =              /* д������*/
"��ӭʹ��Ұ��STM32 F429������ �����Ǹ������ӣ��½��ļ�ϵͳ�����ļ�\r\n";  


uint8_t cnt;
void sd_mount(void)
{
	res_sd = f_mount(&fs,"0:",1);

	if(res_sd == FR_NO_FILESYSTEM)
	{
		printf("��SD����û���ļ�ϵͳ���������и�ʽ��...\r\n");
		/* ��ʽ�� */
		res_sd=f_mkfs("0:",0,0);							
		
		if(res_sd == FR_OK)
		{
			printf("��SD���ѳɹ���ʽ���ļ�ϵͳ��\r\n");
			/* ��ʽ������ȡ������ */
			res_sd = f_mount(NULL,"0:",1);			
			/* ���¹���	*/			
			res_sd = f_mount(&fs,"0:",1);
		}
		else
		{
			printf("������ʽ��ʧ�ܡ�����\r\n");
			while(1);
		}
	}
	else if(res_sd!=FR_OK)
	{
		printf("����SD�������ļ�ϵͳʧ�ܡ�(%d)\r\n",res_sd);
		printf("��������ԭ��SD����ʼ�����ɹ���\r\n");
		while(1);
	}
	else
	{
		printf("���ļ�ϵͳ���سɹ������Խ��ж�д����\r\n");
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
		printf("������/�����ļ�ʧ�ܡ�\r\n");
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
				printf("�����ļ�д��ʧ�ܣ�(%d)\n",res_sd);
			}    
			printf("cnt=%d\r\n",cnt);
		}
		cnt++;
	}
}



/*********************************************END OF FILE**********************/

