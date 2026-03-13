#include "Custom_SPI_Device.h"
#include "board_pins.h"
#include "drivers/rpi_proto.h"

SPI_HandleTypeDef hspi2;
DMA_HandleTypeDef hdma_spi2_rx;
DMA_HandleTypeDef hdma_spi2_tx;

extern uint8_t spi_tx_buf[SPI_TX_BUF_SIZE];
extern uint8_t spi_rx_buf[SPI_RX_BUF_SIZE];
extern uint16_t spi_tx_cnt;

void custom_spi_init(void)
{
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_SLAVE;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 7;
  HAL_SPI_Init(&hspi2);
}

void custom_spi_start(void)
{
  RPI_Protocol_PrepareTX(26);
  HAL_SPI_TransmitReceive_DMA(&hspi2, spi_tx_buf, spi_rx_buf, SPI_RX_BUF_SIZE);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == GPIO_PIN_12) {
    if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12) == GPIO_PIN_SET) {
        /* CS Rise - End of SPI Frame */
        HAL_SPI_DMAStop(&hspi2);
        uint16_t received_len = SPI_RX_BUF_SIZE - __HAL_DMA_GET_COUNTER(hspi2.hdmarx);
        
        if (received_len > 4U) {
            RPI_Protocol_Parse(spi_rx_buf, received_len);
        }
        
        RPI_Protocol_PrepareTX(26);
        HAL_SPI_TransmitReceive_DMA(&hspi2, spi_tx_buf, spi_rx_buf, SPI_RX_BUF_SIZE);
    }
  }
}

