/**
 * @file sys_timer.cpp
 * @brief Система измерения времени выполнения циклов
 *
 * @details
 * Система предназначена для измерения времени между последовательными вызовами функций.
 * Используется для отладки производительности, контроля частоты выполнения циклов
 * и мониторинга временных характеристик в реальном времени.
 */

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include "sys_time.h"

volatile float Cycle_T[GET_TIME_NUM][4];
struct timeval tv;

/**
 * @brief Получить системное время в микросекундах
 *
 * @return uint32_t Текущее время в микросекундах
 *
 * @details
 * Использует gettimeofday() для получения точного времени.
 * Формула: секунды × 1,000,000 + микросекунды
 */
uint32_t GetSysTime_us(void)
{
	register uint32_t ms;
	uint32_t value;
	gettimeofday(&tv, NULL);
	ms = tv.tv_sec * 1000000 + tv.tv_usec;
	value = ms;
	return value;
}

/**
 * @brief Вычислить время между текущим и предыдущим вызовом
 *
 * @param item Индекс измеряемого элемента
 * @return float Разница времени в секундах
 *
 * @details
 * - Сохраняет старое значение времени
 * - Получает текущее время
 * - Вычисляет дельту между вызовами
 * - Обрабатывает случай переполнения таймера
 */
float Get_Cycle_T(int item)
{
	Cycle_T[item][OLD] = Cycle_T[item][NOW];
	Cycle_T[item][NOW] = (float)GetSysTime_us() / 1000000.0f;
	if (Cycle_T[item][NOW] > Cycle_T[item][OLD])
	{
		Cycle_T[item][NEW] = ((Cycle_T[item][NOW] - Cycle_T[item][OLD]));
		Cycle_T[item][DT_LAST] = Cycle_T[item][NEW];
	}
	else
		Cycle_T[item][NEW] = Cycle_T[item][DT_LAST];

	return Cycle_T[item][NEW];
}

/**
 * @brief Инициализация системы измерения времени
 *
 * @details
 * Инициализирует все элементы массива Cycle_T путем вызова
 * Get_Cycle_T() для каждого индекса.
 */
void Cycle_Time_Init()
{
	int i;
	for (i = 0; i < GET_TIME_NUM; i++)
	{
		Get_Cycle_T(i);
	}
}