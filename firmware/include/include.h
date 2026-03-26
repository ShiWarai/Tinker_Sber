#ifndef _INCLUDE_H_
#define _INCLUDE_H_
// Тип платы / опции сборки
#define USE_OLED     0
#define USE_AUDIO    0 //
#define USE_SERVO1   0 // сервопривод
#define USE_VR       0 
// Опции сборки MOCO8 (плоскость, imu_att.z=90° и т.д.)
#define MCU_BIND1    0 
#define MCU_BIND2    0// вариант Tinymal-B C Wheel / GhostV2
#define MCU_BIND3    0// 8 приводов
#define MCU_TINYMAL  0// mini Tinymal
#define MCU_TINKER   1// текущая плата Tinker


#define MCU_TINYPALE_S1 0// Tinypale S1
#define MCU_TINYPALE 0// 485 / шасси
#define USE_IMU_O    0

#define SPI_MESS_TYPE_3BLDC_DIV 1
#define DIV_Q_RESET 0// сброс калибровки q
extern int spi_comm_mess_type;// версия протокола SPI
//---- датчики ноги
#define RUN_WEBOTS 0
#define RUN_PI     0
#define USE_ARM_CAL 1
#define USE_USE_COMM 0

#define EN_AUTO_TORT 0
#define EN_AUTO_ATT_TROT 0
#define EN_AUTO_FORWARD_TROT 0
#define EN_AUTO_ROTATE_TROT 0

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265f
#endif

#ifndef M_PI_2
#define M_PI_2 (M_PI / 2.0f)
#endif

#if !RUN_WEBOTS
#include <stm32f4xx.h>	    
#include "time.h"
#include "mems.h"
#include "parameter.h"
#include "delay.h"   
#include "rc_mine.h"													
#include "flash.h"
#include "dma.h"
#include "rng.h" 
#endif


#define BOARD_V20 

#define MEM_ICM

#define  BOARD_FOR_CAN // связь с RPi по SPI (CAN-плата)

#define  MOCODRIVE 1

//#define LEG_USE_VLX 
//#define LEG_USE_VL6
#define LEG_USE_SWITCH
//#define LEG_USE_AD
#define HARDWARE_V1 
//#define USE_MAVLINK
#define USE_OLDX_REMOTER
#define USE_BLDC
//#define USE_GIMBAL
//#define USE_SERIAL_SERVO
//#define USE_PS3_RC

//#define ATT_EKF
#define ATT_MAD // Madgwick, устойчив на границах
//#define ATT_COM

#define EN_BEEP
//#define EN_ARM

#define FORCE_UP_FULL 1// принудительно полный OCU
/*************** Приоритеты NVIC ******************/
#define NVIC_GROUP NVIC_PriorityGroup_2		// группа приоритетов
#define NVIC_PWMIN_P			1		// приём PWM
#define NVIC_PWMIN_S			1
#define NVIC_PWMIN_S2			0
#define NVIC_TIME_P             2		// не используется
#define NVIC_TIME_S             0
#define NVIC_UART_P				5		// не используется
#define NVIC_UART_S				1
#define NVIC_UART2_P			3		// UART2
#define NVIC_UART2_S			1
/***********************************************/
#define RC_PITCH  1
#define RC_ROLL   0
#define RC_YAW    3
#define RC_MODE   6
#define RC_THR    2
#define RC_GEAR   4
//================ IMU ===================
#define OFFSET_AV_NUM 	50					// усреднение при калибровке смещения
#define FILTER_NUM 			10					// глубина скользящего среднего

#define TO_ANGLE 				0.06103f 		// 4000/65536, ±2000

#define FIX_GYRO_Y 			1.02f				// поправка оси Y гироскопа
#define FIX_GYRO_X 			1.02f				// поправка оси X гироскопа

#define TO_M_S2 				0.23926f   	// 980 см/с², ±8g, 980/4096
#define ANGLE_TO_RADIAN 0.01745329f // градусы -> радианы

#define MAX_ACC  4096.0f						// ±8g, полная шкала акселерометра
#define TO_DEG_S 500.0f      				// T=2 мс по умолчанию; масштаб 1/T
//================ Углы управления =====================
#define MAX_CTRL_ANGLE			25.0f										// макс. угол с RC
#define ANGLE_TO_MAX_AS 		30.0f										// при большом угле — ограничение угловой скорости (внешний контур)
#define CTRL_2_INT_LIMIT 		0.5f *MAX_CTRL_ANGLE		// антивиндуп внешнего контура
#define MAX_FIX_ANGLE 6

#define MAX_CTRL_ASPEED 	 	300.0f									// макс. скорость крена/тангажа
#define MAX_CTRL_YAW_SPEED 	30.0f									// макс. скорость рыскания
#define CTRL_1_INT_LIMIT 		0.5f *MAX_CTRL_ASPEED		// антивиндуп внутреннего контура
//================= PWM ========================
#define MAX_PWM				100			// % макс. PWM
#define MAX_THR       80 			// % газ до 80%, 20% резерв
#define READY_SPEED   20			// % минимальные обороты «готов»
//================ Система ===================
#define GET_TIME_NUM 	(100)		// число замеров dt
#define USE_TOE_IN_UNLOCK 0 // 0 — разблокировка по умолчанию; 1 — иной режим
//============== DMA UART =========================
#define EN_DMA_UART1 0  //
#define EN_DMA_UART2 0  //
#define EN_DMA_UART3 0  //
#define EN_DMA_UART4 0  //
#define EN_DMA_UART6 0  //
 

#define RtA 		57.324841				
#define AtR    	0.0174533				
#define Acc_G 	0.0011963				
#define Gyro_G 	0.03051756				
#define Gyro_Gr	0.0005426

#define A_X 0
#define A_Y 1
#define A_Z 2
#define G_Y 3
#define G_X 4
#define G_Z 5
#define TEM 6
#define ITEMS 7


enum
{
	SETBIT0 = 0x0001,  SETBIT1 = 0x0002,	SETBIT2 = 0x0004,	 SETBIT3 = 0x0008,
	SETBIT4 = 0x0010,	 SETBIT5 = 0x0020,	SETBIT6 = 0x0040,	 SETBIT7 = 0x0080,
	SETBIT8 = 0x0100,	 SETBIT9 = 0x0200,	SETBIT10 = 0x0400, SETBIT11 = 0x0800,
	SETBIT12 = 0x1000, SETBIT13 = 0x2000,	SETBIT14 = 0x4000, SETBIT15 = 0x8000		
};
// Сброс бита: a &= CLRBIT0
enum
{
	CLRBIT0 = 0xFFFE,  CLRBIT1 = 0xFFFD,	CLRBIT2 = 0xFFFB,	 CLRBIT3 = 0xFFF7,	
	CLRBIT4 = 0xFFEF,	 CLRBIT5 = 0xFFDF,	CLRBIT6 = 0xFFBF,	 CLRBIT7 = 0xFF7F,
	CLRBIT8 = 0xFEFF,	 CLRBIT9 = 0xFDFF,	CLRBIT10 = 0xFBFF, CLRBIT11 = 0xF7FF,
	CLRBIT12 = 0xEFFF, CLRBIT13 = 0xDFFF,	CLRBIT14 = 0xBFFF, CLRBIT15 = 0x7FFF
};
// Маска бита: a = b & CHSBIT0
enum
{
	CHSBIT0 = 0x0001,  CHSBIT1 = 0x0002,	CHSBIT2 = 0x0004,	 CHSBIT3 = 0x0008,
	CHSBIT4 = 0x0010,	 CHSBIT5 = 0x0020,	CHSBIT6 = 0x0040,	 CHSBIT7 = 0x0080,
	CHSBIT8 = 0x0100,	 CHSBIT9 = 0x0200,	CHSBIT10 = 0x0400, CHSBIT11 = 0x0800,
	CHSBIT12 = 0x1000, CHSBIT13 = 0x2000,	CHSBIT14 = 0x4000, CHSBIT15 = 0x8000		
};

#define BYTE0(dwTemp)       (*(char *)(&dwTemp))
#define BYTE1(dwTemp)       (*((char *)(&dwTemp) + 1))
#define BYTE2(dwTemp)       (*((char *)(&dwTemp) + 2))
#define BYTE3(dwTemp)       (*((char *)(&dwTemp) + 3))


// Бит-бэндинг GPIO (аналог 8051), см. CM3/CM4 Architecture — раздел про bit-band.
// Макросы для одного бита порта
#define BITBAND(addr, bitnum) ((addr & 0xF0000000)+0x2000000+((addr &0xFFFFF)<<5)+(bitnum<<2)) 
#define MEM_ADDR(addr)  *((volatile unsigned long  *)(addr)) 
#define BIT_ADDR(addr, bitnum)   MEM_ADDR(BITBAND(addr, bitnum)) 

#if !RUN_WEBOTS
// Адреса регистров GPIO для bit-band
#define GPIOA_ODR_Addr    (GPIOA_BASE+20) //0x40020014
#define GPIOB_ODR_Addr    (GPIOB_BASE+20) //0x40020414 
#define GPIOC_ODR_Addr    (GPIOC_BASE+20) //0x40020814 
#define GPIOD_ODR_Addr    (GPIOD_BASE+20) //0x40020C14 
#define GPIOE_ODR_Addr    (GPIOE_BASE+20) //0x40021014 
#define GPIOF_ODR_Addr    (GPIOF_BASE+20) //0x40021414    
#define GPIOG_ODR_Addr    (GPIOG_BASE+20) //0x40021814   
#define GPIOH_ODR_Addr    (GPIOH_BASE+20) //0x40021C14    
#define GPIOI_ODR_Addr    (GPIOI_BASE+20) //0x40022014     

#define GPIOA_IDR_Addr    (GPIOA_BASE+16) //0x40020010 
#define GPIOB_IDR_Addr    (GPIOB_BASE+16) //0x40020410 
#define GPIOC_IDR_Addr    (GPIOC_BASE+16) //0x40020810 
#define GPIOD_IDR_Addr    (GPIOD_BASE+16) //0x40020C10 
#define GPIOE_IDR_Addr    (GPIOE_BASE+16) //0x40021010 
#define GPIOF_IDR_Addr    (GPIOF_BASE+16) //0x40021410 
#define GPIOG_IDR_Addr    (GPIOG_BASE+16) //0x40021810 
#define GPIOH_IDR_Addr    (GPIOH_BASE+16) //0x40021C10 
#define GPIOI_IDR_Addr    (GPIOI_BASE+16) //0x40022010 
 
// Выход/вход одного бита; n < 16
#define PAout(n)   BIT_ADDR(GPIOA_ODR_Addr,n)  // выход
#define PAin(n)    BIT_ADDR(GPIOA_IDR_Addr,n)  // вход

#define PBout(n)   BIT_ADDR(GPIOB_ODR_Addr,n)
#define PBin(n)    BIT_ADDR(GPIOB_IDR_Addr,n)

#define PCout(n)   BIT_ADDR(GPIOC_ODR_Addr,n)
#define PCin(n)    BIT_ADDR(GPIOC_IDR_Addr,n)

#define PDout(n)   BIT_ADDR(GPIOD_ODR_Addr,n)
#define PDin(n)    BIT_ADDR(GPIOD_IDR_Addr,n)

#define PEout(n)   BIT_ADDR(GPIOE_ODR_Addr,n)
#define PEin(n)    BIT_ADDR(GPIOE_IDR_Addr,n)

#define PFout(n)   BIT_ADDR(GPIOF_ODR_Addr,n)
#define PFin(n)    BIT_ADDR(GPIOF_IDR_Addr,n)

#define PGout(n)   BIT_ADDR(GPIOG_ODR_Addr,n)
#define PGin(n)    BIT_ADDR(GPIOG_IDR_Addr,n)

#define PHout(n)   BIT_ADDR(GPIOH_ODR_Addr,n)
#define PHin(n)    BIT_ADDR(GPIOH_IDR_Addr,n)

#define PIout(n)   BIT_ADDR(GPIOI_ODR_Addr,n)
#define PIin(n)    BIT_ADDR(GPIOI_IDR_Addr,n)
#else
#define MEMS_RIGHT_BEEP 0
#define MEMS_ERROR_BEEP 1
#define START_BEEP 2
#define BAT_ERO_BEEP 3
#define RC_ERO_BEEP 4
#define BEEP_ONE 5
#define BEEP_TWO 6
#define BEEP_THREE 7
#define BEEP_STATE 8
#define BEEP_GPS_SAVE 9
#define BEEP_HML_CAL 10
#define BEEP_MISSION 11
#define MEMS_WAY_UPDATE 12
#define MEMS_GPS_RIGHT 14
#define BEEP_DJ_CAL1 15
#define BEEP_DJ_CAL2 16

#define BEEP_BLDC_ZERO_CAL 17
#define BEEP_BLDC_ZERO_INIT 18
#define BEEP_BLDC_GAIT_SWITCH 19
#define BEEP_BLDC_STATE 20
#define BEEP_BLDC_RESET_ERR 21
#endif

#endif

