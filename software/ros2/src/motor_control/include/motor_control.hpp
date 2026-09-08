#ifndef MOTOR_CONTROL_MOTOR_CONTROL_HPP
#define MOTOR_CONTROL_MOTOR_CONTROL_HPP

#include <cstdint>
#include "spi_node.hpp"

namespace motor_control {

/**
 * Формирует пакет команд в буфер SPI и заполняет spi_tx_buf.
 * Вызывать перед SPIDataRW.
 */
void can_board_send(char sel, const _SPI_TX &tx_data, const _MEMS &mems_data);

/**
 * Парсит принятый SPI-пакет в структуру _SPI_RX.
 * @return 1 при успехе, 0 при ошибке
 */
int slave_rx(uint8_t *data_buf, int num, _SPI_RX &rx_out);

/**
 * Один цикл обмена по SPI: can_board_send → SPIDataRW → парсинг в rx_out.
 * @return результат SPIDataRW (число байт или -1 при ошибке)
 */
int spi_transfer_and_parse(int sel, const _SPI_TX &tx_data, const _MEMS &mems_data, _SPI_RX &rx_out);

/** Скорость SPI (Гц) для SPISetup */
uint32_t get_spi_speed();

} // namespace motor_control

#endif /* MOTOR_CONTROL_MOTOR_CONTROL_HPP */
