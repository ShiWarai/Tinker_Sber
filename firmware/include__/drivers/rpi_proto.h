#ifndef RPI_PROTO_H
#define RPI_PROTO_H

#include "app_main.h"

// Constants from legacy can.h/spi.c
#define CAN_T_DIV       500.0f
#define CAN_POS_DIV     30.0f
#define CAN_DPOS_DIV    20.0f
#define CAN_GAIN_DIV_P  500.0f
#define CAN_GAIN_DIV_D  1000.0f

#define SPI_RX_BUF_SIZE 256
#define SPI_TX_BUF_SIZE 256

extern uint32_t spi_rx_cnt_all;
extern uint8_t spi_master_connect_pi;

void RPI_Protocol_Init(void);
void RPI_Protocol_Parse(uint8_t *data, uint16_t len);
void RPI_Protocol_PrepareTX(uint8_t sel);
void RPI_Protocol_LinkLost(void);

#endif /* RPI_PROTO_H */
