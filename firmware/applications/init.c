#include "include.h" 
#include "flash.h"
#include "led_fc.h"
#include "usart_fc.h"
#include "spi.h"
#include "bat.h"
#include "beep.h"
#include "dog.h"
#include "pwm_in.h"
#include "pwm_out.h"
#include "stm32f4xx_dma.h"
#include "pressure.h"
#include "icm20602.h"
#include "can.h"
#include "scheduler.h"
#include "locomotion_header.h"
#include "Custom_SPI_Device.h"
#include "wsled.h"

u8 All_Init()
{
		char i;
		NVIC_PriorityGroupConfig(NVIC_GROUP);//??????????????????2
		SysTick_Configuration(); 	
	 	POWER_INIT();
		RNG_Init();
		PWM_AUX_Out_Init(50); 	 
		PWM_Out_Init(50); 
		LED_Init();			    //????LED
//------------------------Uart Init-------------------------------------
  #if USE_AUDIO
		Usart1_Init(115200);//AI audio
	#else
		Usart1_Init(9600);//wheel 485| remote 2.4G
	#endif

	#if !USE_OLED 
		Usart2_Init(1000000);
	#endif
		Usart3_Init(512000);//omnihub-extcan for expand IO
	  Uart6_Init(115200);	//485 only output
		#if EN_DMA_UART1 
		MYDMA_Config(DMA2_Stream7,DMA_Channel_4,(u32)&USART1->DR,(u32)SendBuff1,SEND_BUF_SIZE1,1);//DMA2,STEAM7,CH4,?????????1,?????SendBuff,?????:SEND_BUF_SIZE.
		#endif
		#if EN_DMA_UART3
		MYDMA_Config(DMA1_Stream3,DMA_Channel_4,(u32)&USART3->DR,(u32)SendBuff3,SEND_BUF_SIZE3+2,0);//DMA2,STEAM7,CH4,?????????1,?????SendBuff,?????:SEND_BUF_SIZE.
		#endif
		#if EN_DMA_UART6 
		MYDMA_Config(DMA2_Stream6,DMA_Channel_5,(u32)&USART6->DR,(u32)SendBuff6,SEND_BUF_SIZE6,1);//DMA2,STEAM7,CH4,?????????1,?????SendBuff,?????:SEND_BUF_SIZE.
		#endif

//-------------------------DMA Init--------------------------

	#if EN_DMA_UART1 
		USART_DMACmd(USART1,USART_DMAReq_Tx,ENABLE);     
		MYDMA_Enable(DMA2_Stream7,SEND_BUF_SIZE1+2);     
	#endif	
	#if EN_DMA_UART3 
		USART_DMACmd(USART3,USART_DMAReq_Tx,ENABLE);       
		MYDMA_Enable(DMA1_Stream3,SEND_BUF_SIZE3+2);    
	#endif
	#if EN_DMA_UART6
		USART_DMACmd(USART6,USART_DMAReq_Tx,ENABLE);     
		MYDMA_Enable(DMA2_Stream6,SEND_BUF_SIZE6+2);     
	#endif	
	//test_ws();//PWM LED
  Delay_ms(100);		
	
 	SPI3_Init();//IMU
	
	#if defined(BOARD_FOR_CAN)&&!USE_USE_COMM
		SPI2_Init();//CAN PI
		Custom_SPI_DEVICE_Slave_Config();
		Custom_SPI_DMABufferConfig();	
		Custom_SPI_DEVICE_Slave_EXTI_Config();
		Custom_SPI_DMABufferStart();
	#endif
  Delay_ms(100);

	icm20602_init();
	W25QXX_Init();
	Delay_ms(100);		
	READ_PARM();Delay_ms(100);
	#if !FLASH_USE_STM32
	
	 READ_WAY_POINTS();
	#endif
	#if defined(EN_BEEP)
	if(spi_master_connect_pi)
		Beep_Init(0,84-1);
	#endif
	
	Adc_Init();
	
	LED_Init_SCL_SDA();//???LED

	//--------------------CAN----------------------------1mbps ???????MIT???
	CAN1_Mode_Init(CAN_SJW_2tq,CAN_BS2_4tq,CAN_BS1_9tq,3,CAN_Mode_Normal);
	CAN2_Mode_Init(CAN_SJW_2tq,CAN_BS2_4tq,CAN_BS1_9tq,3,CAN_Mode_Normal);
	
	CAN_motor_init();

	#if USE_AUDIO
		Write_Audio_Data(0x6);//max audio sound
	#endif
	
	#if USE_SERVO1||USE_VR
		Usart1_Init(1000000);
	#endif
	
	#if defined(BOARD_FOR_CAN)&&1
	int dog_enable=KEY_DOG();
	if(dog_enable||0)
		IWDG_Init(4,500);//100ms
	#endif	
 	return (1);
}
