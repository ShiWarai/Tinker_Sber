/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __CUSTOM_SPI_DEVICE_H
#define __CUSTOM_SPI_DEVICE_H

/* Platform config -----------------------------------------------------------*/

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"
#include <stdio.h>
#include "string.h"

#define USE_WHEEL 0
/* Define config -------------------------------------------------------------*/
#define CAN_LINK_COMM_VER1 1 // new version for param divded ��Э��  ���������Ҫ�ڽڵ����������

#define SEND_DIV_SPI 0
#define SPI_IS_BIG 1
#define SLAVE_SPI_BAUDRATE (SPI_BaudRatePrescaler_8) // ����  SPI��Linux��Ϊ��������
#define DataSize 160                                 // SPI transfer size; must match Linux/RPi side
#define CheckSumSize (0)

#define SPI_DEVICE SPI2
#define SPI_DEVICE_CLK RCC_APB1Periph_SPI2
#define SPI_DEVICE_CLK_INIT RCC_APB1PeriphClockCmd

#define SPI_DEVICE_SCK_PIN GPIO_Pin_13
#define SPI_DEVICE_SCK_GPIO_PORT GPIOB
#define SPI_DEVICE_SCK_GPIO_SOURCE GPIO_PinSource13
#define SPI_DEVICE_SCK_GPIO_AF GPIO_AF_SPI2
#define SPI_DEVICE_SCK_GPIO_CLK RCC_AHB1Periph_GPIOB

#define SPI_DEVICE_MISO_PIN GPIO_Pin_14
#define SPI_DEVICE_MISO_GPIO_PORT GPIOB
#define SPI_DEVICE_MISO_GPIO_SOURCE GPIO_PinSource14
#define SPI_DEVICE_MISO_GPIO_AF GPIO_AF_SPI2
#define SPI_DEVICE_MISO_GPIO_CLK RCC_AHB1Periph_GPIOB

#define SPI_DEVICE_MOSI_PIN GPIO_Pin_15
#define SPI_DEVICE_MOSI_GPIO_PORT GPIOB
#define SPI_DEVICE_MOSI_GPIO_SOURCE GPIO_PinSource15
#define SPI_DEVICE_MOSI_GPIO_AF GPIO_AF_SPI2
#define SPI_DEVICE_MOSI_GPIO_CLK RCC_AHB1Periph_GPIOB

/* Macro ---------------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
void Custom_SPI_DMABufferStart(void);
void Custom_SPI_DMABufferWait(void);
void Custom_SPI_DMABufferConfig(void);

void Custom_SPI_DEVICE_Init(void);
/** Перезапуск SPI2+DMA slave (если зависли до появления мастера) — вызывать из главного цикла, не из IRQ. */
void Custom_SPI_Slave_RecoverDma(void);

/* Exported constants --------------------------------------------------------*/

#endif /* __CUSTOM_SPI_DEVICE_H */
