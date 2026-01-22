/**
 * @file spi.cpp
 * @brief SPI (Serial Peripheral Interface) драйвер для Linux
 *
 * @details
 * Реализует взаимодействие с SPI устройствами через Linux spidev интерфейс.
 * Поддерживает два SPI канала (spidev0.0 и spidev0.1) с настройкой скорости,
 * режима работы и битности данных.
 *
 */

#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <asm/ioctl.h>
#include <linux/spi/spidev.h>
#include "spi.h"

/** @brief Путь к первому SPI устройству */
static const char *spiDev0 = "/dev/spidev0.0";

/** @brief Путь ко второму SPI устройству */
static const char *spiDev1 = "/dev/spidev0.1";

/** @brief Биты на слово (обычно 8) */
static uint8_t spiBPW = 8;

/** @brief Задержка передачи в микросекундах */
static uint16_t spiDelay = 0;

/** @brief Скорости SPI для каждого канала */
static uint32_t spiSpeeds[2];

/** @brief Файловые дескрипторы SPI устройств */
static int spiFds[2];

/**
 * @brief Запись и чтение данных по SPI (полный дуплекс)
 *
 * @param channel Номер канала SPI (0 или 1)
 * @param tx_data Буфер передаваемых данных
 * @param rx_data Буфер для принимаемых данных
 * @param len Количество байт для передачи/приема
 * @return int Результат операции:
 *             - >=0: количество переданных байт
 *             - -1: ошибка
 *
 * @details
 * ВАЖНО: Принимаемые данные записываются в буфер передачи, перезаписывая его!
 * Это операция полного дуплекса - передача и прием происходят одновременно.
 *
 * @note Использует структуру spi_ioc_transfer для настройки передачи
 */
int SPIDataRW(int channel, uint8_t *tx_data, uint8_t *rx_data, int len)
{
	int i = 0;
	struct spi_ioc_transfer spi;

	channel &= 1; // Обеспечиваем корректный диапазон канала (0 или 1)

	memset(&spi, 0, sizeof(spi));

	spi.tx_buf = (unsigned long)tx_data;
	// Отладочный вывод (закомментирован)
	// printf("%d", len);
	// printf("START\n");
	// for (int i = 0; i < len; i++){
	//     printf("%d\n", tx_data[i]);
	// }
	// printf("STOP\n");

	spi.rx_buf = (unsigned long)rx_data;
	spi.len = len;
	spi.delay_usecs = spiDelay;
	spi.speed_hz = spiSpeeds[channel];
	spi.bits_per_word = spiBPW;

	return ioctl(spiFds[channel], SPI_IOC_MESSAGE(1), &spi); // SPI_IOC_MESSAGE(1) - 1 означает количество spi_ioc_transfer структур
}

/**
 * @brief Настройка SPI устройства с указанием режима
 *
 * @param channel Номер канала SPI (0 или 1)
 * @param speed Скорость SPI в Гц
 * @param mode Режим SPI:
 *             - SPI_MODE_0: CPOL=0, CPHA=0
 *             - SPI_MODE_1: CPOL=0, CPHA=1
 *             - SPI_MODE_2: CPOL=1, CPHA=0
 *             - SPI_MODE_3: CPOL=1, CPHA=1
 * @return int Файловый дескриптор устройства или -1 при ошибке
 *
 * @details
 * Выполняет полную настройку SPI устройства:
 * - Открывает устройство
 * - Устанавливает режим работы
 * - Настраивает битность данных
 * - Устанавливает скорость передачи
 *
 * @warning При ошибке выводит сообщение в stderr
 */
int SPISetupMode(int channel, int speed, int mode)
{
	int fd;

	// Открываем SPI устройство
	if ((fd = open(channel == 0 ? spiDev0 : spiDev1, O_RDWR)) < 0)
	{
		printf("Unable to open SPI device: %s\n", strerror(errno));
		return -1;
	}

	spiSpeeds[channel] = speed;
	spiFds[channel] = fd;

	// Установка режима SPI
	if (ioctl(fd, SPI_IOC_WR_MODE, &mode) < 0)
	{
		printf("Can't set spi mode: %s\n", strerror(errno));
		return -1;
	}

	// Чтение режима SPI для проверки
	if (ioctl(fd, SPI_IOC_RD_MODE, &mode) < 0)
	{
		printf("Can't get spi mode: %s\n", strerror(errno));
		return -1;
	}

	// Установка битности данных
	if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &spiBPW) < 0)
	{
		printf("Can't set bits per word: %s\n", strerror(errno));
		return -1;
	}

	// Чтение битности данных для проверки
	if (ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &spiBPW) < 0)
	{
		printf("Can't get bits per word: %s\n", strerror(errno));
		return -1;
	}

	// Установка максимальной скорости
	if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0)
	{
		printf("Can't set max speed hz: %s\n", strerror(errno));
		return -1;
	}

	// Чтение скорости для проверки
	if (ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed) < 0)
	{
		printf("Can't get max speed hz: %s\n", strerror(errno));
		return -1;
	}

	return fd;
}

/**
 * @brief Упрощенная настройка SPI устройства (режим 0 по умолчанию)
 *
 * @param channel Номер канала SPI (0 или 1)
 * @param speed Скорость SPI в Гц
 * @return int Файловый дескриптор устройства или -1 при ошибке
 *
 * @details
 * Использует режим SPI_MODE_0 (CPOL=0, CPHA=0) по умолчанию.
 * Вызывает SPISetupMode с предустановленным режимом.
 */
int SPISetup(int channel, int speed)
{
	return SPISetupMode(channel, speed, SPI_MODE_0);
}