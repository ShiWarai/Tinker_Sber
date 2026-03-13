#ifndef CUSTOM_SPI_DEVICE_H
#define CUSTOM_SPI_DEVICE_H

#include <stdint.h>
#include "stm32f4xx_hal.h"

#define CUSTOM_SPI_DATASIZE 162
#define CUSTOM_SPI_CHECKSUMSIZE 2

extern uint8_t DataRxBuffer[CUSTOM_SPI_DATASIZE + CUSTOM_SPI_CHECKSUMSIZE];
extern uint8_t DataTxBuffer[CUSTOM_SPI_DATASIZE + CUSTOM_SPI_CHECKSUMSIZE];

void custom_spi_init(void);
void custom_spi_start(void);

#endif /* CUSTOM_SPI_DEVICE_H */
