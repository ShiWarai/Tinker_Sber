//////////////////////////////////////////////////////////////////////////////////
// Учебный пример; авторы не отвечают за потерю управления при применении копий.
// OLDX-AutoPilot
// Декодер SBUS
// Дата: 2017/4/15
// Версия: V1.0
// Все права защищены.
// Copyright(C) BIT Yunyi Innovation Team 2016-2024
// All rights reserved
//********************************************************************************

//////////////////////////////////////////////////////////////////////////////////

#include "stm32f4xx.h"
extern int16_t channels[18];    // выход каналов SBUS
extern uint8_t failsafe_status; // статус сигнала / failsafe
//------------------------------------------------//
extern uint8_t sbus_data_i[26];
extern uint8_t sbus_data[26];
extern uint8_t sbus_passthrough;
#define SBUS_SIGNAL_OK 0x00
#define SBUS_SIGNAL_LOST 0x01
#define SBUS_SIGNAL_FAILSAFE 0x03
//------------------------------------------------//
void oldx_sbus_rx(u8 com_data); // вызывать из прерывания UART SBUS
