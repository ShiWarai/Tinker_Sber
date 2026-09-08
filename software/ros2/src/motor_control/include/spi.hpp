#ifndef MOTOR_CONTROL_SPI_HPP
#define MOTOR_CONTROL_SPI_HPP

#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

int SPIDataRW(int channel, uint8_t *tx_data, uint8_t *rx_data, int len);
int SPISetupMode(int channel, int speed, int mode);
int SPISetup(int channel, int speed);

#ifdef __cplusplus
}
#endif

#endif /* MOTOR_CONTROL_SPI_HPP */
