#include "led_fc.h"
#include "include.h"
#include "mems.h"
#include "beep.h"
#include "usart_fc.h"
#include "gait_math.h"
#include "spi.h"
#include "can.h"
int dog_flag;
int KEY_DOG(void)
{
#if defined(BOARD_FOR_CAN) && !USE_USE_COMM
	/* Не трогаем режим PB12: на CAN-плате после SPI3_Init там AF (RS485). */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	dog_flag = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_12);
	return dog_flag;
#else
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	dog_flag = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_12);
	return dog_flag;
#endif
}

void POWER_INIT(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	leg_power_control(0);
}

void leg_power_control(u8 sel)
{
	if (sel)
		GPIO_ResetBits(GPIOC, GPIO_Pin_5);
	else
		GPIO_SetBits(GPIOC, GPIO_Pin_5);
}

void LEDRGB_COLOR(u8 color);

void LED_ConfigIndicatorPins(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOA, ENABLE);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
}

void LED_DELAY(float delay)
{
	long i = 0;
	int num = delay * 3000000;
	for (i = 0; i < num; i++)
	{
		__NOP();
		__NOP();
		__NOP();
		__NOP();
		__NOP();
		__NOP();
		__NOP();
		__NOP();
		__NOP();
	}
}

void LED_Init()
{
	char i = 0;
	GPIO_InitTypeDef GPIO_InitStructure;

	/* Раньше: GPIO_Init(GPIOB) до RCC → режим PB3/PB4 не записывался; после SPI3_Init такт B есть, но линии оставались IN */
	LED_ConfigIndicatorPins();

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	for (i = 0; i < 2; i++)
	{
		LEDRGB_COLOR(WHITE);
		LED_DELAY(0.25);
		LEDRGB_COLOR(BLACK);
		LED_DELAY(0.25);
	}
	LEDRGB_ST(RED, 1);
	LEDRGB_ST(BLUE, 1);
	LED_DELAY(0.25);
	LEDRGB_ST(RED, 0);
	LEDRGB_ST(BLUE, 0);
}

void LEDRGB_ST(u8 sel, u8 on)
{
	switch (sel)
	{
	case RED:
		if (!on)
			GPIO_ResetBits(GPIOA, GPIO_Pin_0);
		else
			GPIO_SetBits(GPIOA, GPIO_Pin_0);
		break;
	case BLUE:
		if (!on)
			GPIO_ResetBits(GPIOA, GPIO_Pin_1);
		else
			GPIO_SetBits(GPIOA, GPIO_Pin_1);
		break;
	}
}

void LEDRGB(u8 sel, u8 on)
{
	switch (sel)
	{
	case RED:
		/* PB3 — как на старых платах; PA0 — пара «ST» из LED_Init, часто это и есть видимые LED */
		if (!on)
		{
			GPIO_ResetBits(GPIOB, GPIO_Pin_3);
			GPIO_ResetBits(GPIOA, GPIO_Pin_0);
		}
		else
		{
			GPIO_SetBits(GPIOB, GPIO_Pin_3);
			GPIO_SetBits(GPIOA, GPIO_Pin_0);
		}
		break;
	}
}

void LEDRGB_RED(u8 on)
{
	if (!on)
	{
		GPIO_ResetBits(GPIOB, GPIO_Pin_3);
		GPIO_ResetBits(GPIOA, GPIO_Pin_0);
	}
	else
	{
		GPIO_SetBits(GPIOB, GPIO_Pin_3);
		GPIO_SetBits(GPIOA, GPIO_Pin_0);
	}
}

void LEDRGB_BLUE(u8 on)
{
	if (!on)
	{
		GPIO_ResetBits(GPIOB, GPIO_Pin_4);
		GPIO_ResetBits(GPIOA, GPIO_Pin_1);
	}
	else
	{
		GPIO_SetBits(GPIOB, GPIO_Pin_4);
		GPIO_SetBits(GPIOA, GPIO_Pin_1);
	}
}

u8 LED[3] = {0};
void LEDRGB_COLOR(u8 color)
{
	switch (color)
	{
	case WHITE:
		LED[0] = 1;
		LED[1] = 0;
		LED[2] = 0;
		LEDRGB(RED, 1);
		break;
	case BLACK:
		LED[0] = 0;
		LED[1] = 1;
		LED[2] = 0;
		LEDRGB(RED, 0);
		break;
	}
}

int dj_sel = 0;

/* Красный: связь со SBC по SPI (slave_rx); синий: хотя бы один мотор на CAN с param.connect */
void LEDRGB_STATE(float dt)
{
	(void)dt;
	static u8 main_state_red, main_state_blue;
	static int cnt_can = 0;
	int can_cnt = 0;
	int i;

	if (spi_master_connect_pi)
	{
		if (spi_rx_cnt_all > 100)
		{
			spi_rx_cnt_all = 0;
			main_state_red = !main_state_red;
		}
		if (main_state_red)
			LEDRGB_RED(1);
		else
			LEDRGB_RED(0);
	}
	else
		LEDRGB_RED(0);

	for (i = 0; i < 10; i++)
	{
		if (motor_chassis[i].param.connect)
			can_cnt++;
	}
	if (can_cnt)
	{
		cnt_can++;
		if (cnt_can > 120 - can_cnt * 10)
		{
			cnt_can = 0;
			main_state_blue = !main_state_blue;
		}
		if (main_state_blue)
			LEDRGB_BLUE(1);
		else
			LEDRGB_BLUE(0);
	}
	else
		LEDRGB_BLUE(0);
}
char thread_running[3] = {0};
char io_sel_scp_scl[2] = {0};
void LED_SCP(u8 on)
{
	if (!on)
		GPIO_ResetBits(GPIOA, GPIO_Pin_7);
	else
		GPIO_SetBits(GPIOA, GPIO_Pin_7);
}

void LED_SCL(u8 on)
{
	if (!on)
		GPIO_ResetBits(GPIOA, GPIO_Pin_6);
	else
		GPIO_SetBits(GPIOA, GPIO_Pin_6);
}

void LED_Init_SCL_SDA()
{
	char i = 0;
	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	LED_SCP(0);
	LED_SCL(0);
}