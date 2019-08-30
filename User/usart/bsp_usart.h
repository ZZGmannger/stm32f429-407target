#ifndef __DEBUG_USART_H
#define	__DEBUG_USART_H

#include "stm32f4xx.h"
#include <stdio.h>

#define DEBUG_USART_EN
#define CMD_USART_EN


//引脚定义
/*******************************************************/
#define DEBUG_USART                             USART1
#define DEBUG_USART_CLK                         RCC_APB2Periph_USART1
#define DEBUG_USART_BAUDRATE                    115200  //串口波特率

#define DEBUG_USART_RX_GPIO_PORT                GPIOA
#define DEBUG_USART_RX_GPIO_CLK                 RCC_AHB1Periph_GPIOA
#define DEBUG_USART_RX_PIN                      GPIO_Pin_10
#define DEBUG_USART_RX_AF                       GPIO_AF_USART1
#define DEBUG_USART_RX_SOURCE                   GPIO_PinSource10

#define DEBUG_USART_TX_GPIO_PORT                GPIOA
#define DEBUG_USART_TX_GPIO_CLK                 RCC_AHB1Periph_GPIOA
#define DEBUG_USART_TX_PIN                      GPIO_Pin_9
#define DEBUG_USART_TX_AF                       GPIO_AF_USART1
#define DEBUG_USART_TX_SOURCE                   GPIO_PinSource9

#define DEBUG_USART_IRQHandler                  USART1_IRQHandler
#define DEBUG_USART_IRQ                 				USART1_IRQn
/************************************************************/
//引脚定义
/*******************************************************/
#define CMD_USART                             USART2
#define CMD_USART_CLK                         RCC_APB1Periph_USART2
#define CMD_USART_BAUDRATE                    115200  //串口波特率

#define CMD_USART_RX_GPIO_PORT                GPIOA
#define CMD_USART_RX_GPIO_CLK                 RCC_AHB1Periph_GPIOA
#define CMD_USART_RX_PIN                      GPIO_Pin_3
#define CMD_USART_RX_AF                       GPIO_AF_USART2
#define CMD_USART_RX_SOURCE                   GPIO_PinSource3

#define CMD_USART_TX_GPIO_PORT                GPIOA
#define CMD_USART_TX_GPIO_CLK                 RCC_AHB1Periph_GPIOA
#define CMD_USART_TX_PIN                      GPIO_Pin_2
#define CMD_USART_TX_AF                       GPIO_AF_USART2
#define CMD_USART_TX_SOURCE                   GPIO_PinSource2


#define CMD_USART_IRQHandler                  USART2_IRQHandler
#define CMD_USART_IRQ                 		  USART2_IRQn


typedef void (*usart_cb_t)(uint8_t *buffer,uint8_t length);
void usart_init(void);
void usart_register(uint8_t port,usart_cb_t cb);
void usart_write(USART_TypeDef * pUSARTx , uint8_t *buffer, uint8_t len);


#endif /* __USART1_H */
