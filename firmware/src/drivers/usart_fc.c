#include "include.h"
#include "usart_fc.h"
#include "beep.h"
#include <stdio.h>
#include <string.h>

_ODOMETER flow;
_PI pi;
MOUDLE module;
RC_GETDATA Rc_Get;
RC_GETDATA Rc_Get_PWM;
RC_GETDATA Rc_Get_SBUS;
RC_GETDATA Rc_Wifi;
M100 m100, px4;
_ARMSS arm_cmd_s;
_IMUO imuo;
_WHEEL_WX _wheel_wx[4];
_WHEEL_2Dof _wheel_2d;
_Robot robot;
_LINK_CMD o_cmd;
_FLOW optical_flow;

float ws_set_flt;
int pwm_dj[5] = {1500, 1500, 1500, 1500, 1500};
int time_dj[5] = {0, 0, 0, 0, 0};
u8 RxState1;
int16_t BLE_DEBUG[16];

void Uart6_Init(u32 br_num)
{
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART6, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

	NVIC_InitStructure.NVIC_IRQChannel = USART6_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	GPIO_PinAFConfig(GPIOC, GPIO_PinSource6, GPIO_AF_USART6);
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource7, GPIO_AF_USART6);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	USART_InitStructure.USART_BaudRate = br_num;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART6, &USART_InitStructure);

	USART_Cmd(USART6, ENABLE);
	USART_ClearFlag(USART6, USART_FLAG_TC);
	USART_ITConfig(USART6, USART_IT_RXNE, ENABLE);
}

void UsartSend6(uint8_t ch)
{
	while (USART_GetFlagStatus(USART6, USART_FLAG_TXE) == RESET);
	USART_SendData(USART6, ch);
}

#define UART6_RX_RING_SIZE 512
static volatile uint8_t uart6_rx_ring[UART6_RX_RING_SIZE];
static volatile uint16_t uart6_rx_head;
static volatile uint16_t uart6_rx_tail;

void Uart6_SendBytes(const uint8_t *data, uint16_t length)
{
	uint16_t i;
	for (i = 0; i < length; i++)
		UsartSend6(data[i]);
}

void Uart6_SendString(const char *str)
{
	while (*str)
		UsartSend6((uint8_t)*str++);
}

uint16_t Uart6_Available(void)
{
	uint16_t head = uart6_rx_head;
	uint16_t tail = uart6_rx_tail;
	if (head >= tail)
		return (uint16_t)(head - tail);
	return (uint16_t)(UART6_RX_RING_SIZE - tail + head);
}

uint16_t Uart6_Read(uint8_t *data, uint16_t max_len)
{
	uint16_t cnt = 0;
	while (cnt < max_len && uart6_rx_tail != uart6_rx_head)
	{
		data[cnt++] = uart6_rx_ring[uart6_rx_tail];
		uart6_rx_tail = (uint16_t)((uart6_rx_tail + 1) % UART6_RX_RING_SIZE);
	}
	return cnt;
}

int fputc(int ch, FILE *f)
{
	(void)f;
	if (ch == '\n')
		UsartSend6('\r');
	UsartSend6((uint8_t)ch);
	return ch;
}

void USART6_IRQHandler(void)
{
	uint8_t data;
	uint16_t next_head;

	if (USART6->SR & USART_SR_ORE)
		(void)USART6->DR;
	if (USART_GetITStatus(USART6, USART_IT_RXNE))
	{
		USART_ClearITPendingBit(USART6, USART_IT_RXNE);
		data = (uint8_t)USART6->DR;
		next_head = (uint16_t)((uart6_rx_head + 1) % UART6_RX_RING_SIZE);
		if (next_head != uart6_rx_tail)
		{
			uart6_rx_ring[uart6_rx_head] = data;
			uart6_rx_head = next_head;
		}
	}
}

#define UART6_LINE_MAX 64
static char uart6_line[UART6_LINE_MAX];
static uint8_t uart6_line_len;

void Uart6_PollCommandLine(void)
{
	uint8_t b;

	while (Uart6_Available())
	{
		if (Uart6_Read(&b, 1) != 1) {
			break;
		}

		if (b == '\r' || b == '\n')
		{
			int cmd;

			uart6_line[uart6_line_len] = '\0';
			cmd = (uart6_line_len > 0 && strcmp(uart6_line, "test") == 0);
			uart6_line_len = 0;
			uart6_line[0] = '\0';
			if (cmd)
			{
				printf("test\n");
				Play_Music_Direct(MEMS_GPS_RIGHT);	
			}
			continue;
		}
		if (uart6_line_len < UART6_LINE_MAX - 1)
			uart6_line[uart6_line_len++] = (char)b;
		else
			uart6_line_len = 0;
	}
}

#if USE_AUDIO
void Write_Audio_Data(uint8_t dat)
{
	(void)dat;
}
#endif
