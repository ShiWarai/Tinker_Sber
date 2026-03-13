#ifndef CAN_H
#define CAN_H

#include <stdint.h>
#include "stm32f4xx_hal.h"
#include "app_main.h"

void can_init(void);
void can_rx_handler(CAN_HandleTypeDef *hcan);
void CAN_motor_sm(float dt);

/* Legacy-compatible API names to simplify further migration steps. */
uint8_t CAN1_Mode_Init(uint8_t tsjw, uint8_t tbs2, uint8_t tbs1, float brp, uint8_t mode);
uint8_t CAN2_Mode_Init(uint8_t tsjw, uint8_t tbs2, uint8_t tbs1, float brp, uint8_t mode);

uint8_t CAN1_Send_Msg(uint8_t *msg, uint8_t len, uint32_t id);
uint8_t CAN2_Send_Msg(uint8_t *msg, uint8_t len, uint32_t id);
uint8_t CAN_Send_MIT(motor_measure_t *ptr);
uint8_t CAN_MIT_Motor_Mode_En(uint8_t id, uint8_t en);

uint8_t CAN1_Receive_Msg(uint8_t *buf);
uint8_t CAN2_Receive_Msg(uint8_t *buf);

extern uint32_t can1_rx_id;
extern uint32_t can2_rx_id;
extern float cnt_rst1;
extern float cnt_rst2;
extern int can1_rx_cnt;
extern int can2_rx_cnt;

void data_can_mit_anal(motor_measure_t *ptr, uint8_t buf_rx[8]);
uint8_t mit_motor_mode_en(uint8_t id, uint8_t en);
uint8_t mit_set_pos_zero(uint8_t id);
char data_can_mit_send(motor_measure_t *ptr);

#endif /* CAN_H */
