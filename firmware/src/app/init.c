#include "include.h"
#include "flash.h"
#include "led_fc.h"
#include "usart_fc.h"
#include "spi.h"
#include "beep.h"
#include "watch_dog.h"
#include "stm32f4xx_dma.h"
#include "icm20602.h"
#include "can.h"
#include "scheduler.h"
#include "Custom_SPI_DEVICE.h"
#include "wsled.h"

u8 All_Init()
{
	char i;
	NVIC_PriorityGroupConfig(NVIC_GROUP); //??????????????????2
	SysTick_Configuration();
	POWER_INIT();
	RNG_Init();
	LED_Init(); //????LED
	            //------------------------Uart Init-------------------------------------
	Uart6_Init(115200);
#if EN_DMA_UART1
	MYDMA_Config(DMA2_Stream7, DMA_Channel_4, (u32)&USART1->DR, (u32)SendBuff1, SEND_BUF_SIZE1,
	             1); // DMA2,STEAM7,CH4,?????????1,?????SendBuff,?????:SEND_BUF_SIZE.
#endif
#if EN_DMA_UART3
	MYDMA_Config(DMA1_Stream3, DMA_Channel_4, (u32)&USART3->DR, (u32)SendBuff3, SEND_BUF_SIZE3 + 2,
	             0); // DMA2,STEAM7,CH4,?????????1,?????SendBuff,?????:SEND_BUF_SIZE.
#endif
#if EN_DMA_UART6
	MYDMA_Config(DMA2_Stream6, DMA_Channel_5, (u32)&USART6->DR, (u32)SendBuff6, SEND_BUF_SIZE6,
	             1); // DMA2,STEAM7,CH4,?????????1,?????SendBuff,?????:SEND_BUF_SIZE.
#endif

	//-------------------------DMA Init--------------------------

#if EN_DMA_UART1
	USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);
	MYDMA_Enable(DMA2_Stream7, SEND_BUF_SIZE1 + 2);
#endif
#if EN_DMA_UART3
	USART_DMACmd(USART3, USART_DMAReq_Tx, ENABLE);
	MYDMA_Enable(DMA1_Stream3, SEND_BUF_SIZE3 + 2);
#endif
#if EN_DMA_UART6
	USART_DMACmd(USART6, USART_DMAReq_Tx, ENABLE);
	MYDMA_Enable(DMA2_Stream6, SEND_BUF_SIZE6 + 2);
#endif
	Delay_ms(100);

	SPI3_Init(); // IMU

#if defined(BOARD_FOR_CAN) && !USE_USE_COMM
	Custom_SPI_DEVICE_Init();
#endif
	Delay_ms(100);

	icm20602_init();
	W25QXX_Init();
	Delay_ms(100);
	READ_PARM();
	Delay_ms(100);
#if defined(EN_BEEP)
	if (spi_master_connect_pi)
		Beep_Init(0, 84 - 1);
#endif

	LED_Init_SCL_SDA(); //???LED

	//--------------------CAN----------------------------1mbps ???????MIT???
	CAN1_Mode_Init(CAN_SJW_2tq, CAN_BS2_4tq, CAN_BS1_9tq, 3, CAN_Mode_Normal);
	CAN2_Mode_Init(CAN_SJW_2tq, CAN_BS2_4tq, CAN_BS1_9tq, 3, CAN_Mode_Normal);

	CAN_motor_init();

#if USE_AUDIO
	Write_Audio_Data(0x6); // max audio sound
#endif

#if defined(BOARD_FOR_CAN) && 1
	int dog_enable = KEY_DOG();
	if (dog_enable || 0)
		IWDG_Init(4, 500); // 100ms
#endif
	return (1);
}
