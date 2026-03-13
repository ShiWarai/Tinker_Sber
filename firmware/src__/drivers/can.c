#include "can.h"
#include "board_pins.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#define CAN_TIMEOUT_MS 500U
#define RESET_Q_REPEAT_COUNT 4U
#define RESET_Q_SETTLE_TIME_S 3.5f
#define CYCLE_TIMER_SLOTS 64U

CAN_HandleTypeDef hcan1;
CAN_HandleTypeDef hcan2;

uint32_t can1_rx_id = 0U;
uint32_t can2_rx_id = 0U;
float cnt_rst1 = 0.0f;
float cnt_rst2 = 0.0f;
int can1_rx_cnt = 0;
int can2_rx_cnt = 0;

extern _LEG_MOTOR leg_motor;
extern motor_measure_t motor_chassis[NUM_MOTORS];

static uint32_t cycle_tick_ms[CYCLE_TIMER_SLOTS];

/* MIT protocol ranges - Read-only calibration data */
static const float P_MIN_CAN_MIT[NUM_MOTORS] = {-12.5f, -12.5f, -12.5f, -12.5f, -12.5f, -12.5f, -12.5f, -12.5f, -12.5f, -12.5f};
static const float P_MAX_CAN_MIT[NUM_MOTORS] = {12.5f, 12.5f, 12.5f, 12.5f, 12.5f, 12.5f, 12.5f, 12.5f, 12.5f, 12.5f};
static const float V_MIN_CAN_MIT[NUM_MOTORS] = {-38.2f, -38.2f, -38.2f, -38.2f, -38.2f, -38.2f, -38.2f, -38.2f, -38.2f, -38.2f};
static const float V_MAX_CAN_MIT[NUM_MOTORS] = {38.2f, 38.2f, 38.2f, 38.2f, 38.2f, 38.2f, 38.2f, 38.2f, 38.2f, 38.2f};
static const float KP_MIN_CAN_MIT[NUM_MOTORS] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
static const float KP_MAX_CAN_MIT[NUM_MOTORS] = {500.0f, 500.0f, 500.0f, 500.0f, 500.0f, 500.0f, 500.0f, 500.0f, 500.0f, 500.0f};
static const float KD_MIN_CAN_MIT[NUM_MOTORS] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
static const float KD_MAX_CAN_MIT[NUM_MOTORS] = {5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f};
static const float T_MIN_CAN_MIT[NUM_MOTORS] = {-12.0f, -12.0f, -12.0f, -12.0f, -12.0f, -12.0f, -12.0f, -12.0f, -12.0f, -12.0f};
static const float T_MAX_CAN_MIT[NUM_MOTORS] = {12.0f, 20.0f, 20.0f, 20.0f, 12.0f, 12.0f, 20.0f, 20.0f, 20.0f, 12.0f};

static void can_init_motor_map(void)
{
  static const motor_type_e motor_types[NUM_MOTORS] = {
    DM_6006, DM_8006, DM_8006, DM_8006, DM_6006,
    DM_6006, DM_8006, DM_8006, DM_8006, DM_6006,
  };
  static const int q_flags[NUM_MOTORS] = {0, 1, 1, 0, 1, 0, 1, 0, 1, 0};

  leg_motor.connect = 0;
  leg_motor.motor_en = 0;
  leg_motor.reset_q = 0;
  leg_motor.reset_err = 0;

  for (uint8_t i = 0U; i < NUM_MOTORS; i++) {
    motor_chassis[i].param.id = i;
    motor_chassis[i].param.control_mode = 0;
    motor_chassis[i].param.q_flag = q_flags[i];
    motor_chassis[i].param.t_inv_flag_measure = 1;
    motor_chassis[i].param.t_inv_flag_cmd = 1;
    motor_chassis[i].motor.type = motor_types[i];
    motor_chassis[i].kp = 0.5f;
    motor_chassis[i].kd = 0.1f;
    motor_chassis[i].max_t = 120.0f;
    leg_motor.max_t[i] = 120.0f;
    leg_motor.connect_motor[i] = 0;
    leg_motor.ready[i] = 0;
  }
}

/* Math Helpers */
float To_PI(float x)
{
  while (x > M_PI) x -= 2.0f * M_PI;
  while (x < -M_PI) x += 2.0f * M_PI;
  return x;
}

float Moving_Median(uint8_t item, uint8_t width_num, float in)
{
  (void)item;
  (void)width_num;
  return in;
}

float Get_Cycle_T(uint8_t item)
{
  uint32_t now_ms;
  uint32_t previous_ms;

  if (item >= CYCLE_TIMER_SLOTS) {
    return 0.001f;
  }

  now_ms = HAL_GetTick();
  previous_ms = cycle_tick_ms[item];
  cycle_tick_ms[item] = now_ms;

  if (previous_ms == 0U || now_ms <= previous_ms) {
    return 0.001f;
  }

  return (float)(now_ms - previous_ms) * 0.001f;
}

static float uint_to_float_mit(int x_int, float x_min, float x_max, int bits)
{
  float span = x_max - x_min;
  float offset = x_min;
  return ((float)x_int) * span / ((float)((1 << bits) - 1)) + offset;
}

static int float_to_uint_mit(float x, float x_min, float x_max, int bits)
{
  float span = x_max - x_min;
  float offset = x_min;
  x = LIMIT(x, x_min, x_max);
  return (int)((x - offset) * ((float)((1 << bits) - 1)) / span);
}

void data_can_mit_anal(motor_measure_t *ptr, uint8_t buf_rx[8])
{
  float dt = Get_Cycle_T((uint8_t)(ptr->param.id + 50U));
  int inv_q_flag = ptr->param.q_flag ? 1 : -1;

  ptr->param.connect = 1;
  ptr->param.loss_cnt = 0;
  ptr->param.rx_dt = dt;
  leg_motor.connect_motor[ptr->param.id] = 1;
  leg_motor.ready[ptr->param.id] = leg_motor.motor_en ? 1 : 0;

  uint16_t p_int = (buf_rx[1] << 8) | buf_rx[2];
  uint16_t v_int = (buf_rx[3] << 4) | (buf_rx[4] >> 4);
  uint16_t i_int = ((buf_rx[4] & 0xFU) << 8) | buf_rx[5];

  ptr->param.total_angle_out = uint_to_float_mit(p_int, P_MIN_CAN_MIT[ptr->param.id], P_MAX_CAN_MIT[ptr->param.id], 16);
  ptr->t_now_flt = uint_to_float_mit(i_int, -T_MAX_CAN_MIT[ptr->param.id], T_MAX_CAN_MIT[ptr->param.id], 12) * ptr->param.t_inv_flag_measure * inv_q_flag;

  float total_angle_out_single = (float)fmod(ptr->param.total_angle_out, (double)(2.0f * M_PI));
  ptr->param.total_angle_out_single = To_PI(total_angle_out_single);

  ptr->q_now = inv_q_flag * ptr->param.total_angle_out_single;
  ptr->q_now_flt = To_PI(ptr->q_now + ptr->param.q_reset_angle);

  float v_float = uint_to_float_mit(v_int, V_MIN_CAN_MIT[ptr->param.id], V_MAX_CAN_MIT[ptr->param.id], 12);
  ptr->qd_now = v_float * inv_q_flag;
  ptr->qd_now_flt = Moving_Median((uint8_t)ptr->param.id, 3U, ptr->qd_now);

  ptr->param.q_now_reg = ptr->q_now;
  ptr->param.qd_now_reg = ptr->qd_now;
}

void can_rx_handler(CAN_HandleTypeDef *hcan)
{
  CAN_RxHeaderTypeDef rx_header;
  uint8_t rx_data[8];

  if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_header, rx_data) == HAL_OK) {
    if ((rx_header.IDE == CAN_ID_STD) && (rx_header.RTR == CAN_RTR_DATA) && (rx_header.DLC == 6 || rx_header.DLC == 8)) {
      uint8_t motor_id = 255U;
      if (hcan->Instance == CAN1) {
        can1_rx_id = rx_header.StdId;
        can1_rx_cnt++;
        motor_id = (uint8_t)(rx_data[0] - 1U);
        if (motor_id < 5U) {
          data_can_mit_anal(&motor_chassis[motor_id], rx_data);
          leg_motor.q_now[motor_id] = motor_chassis[motor_id].q_now_flt;
          leg_motor.qd_now[motor_id] = motor_chassis[motor_id].qd_now_flt;
          leg_motor.t_now[motor_id] = motor_chassis[motor_id].t_now_flt;
        }
      } else {
        can2_rx_id = rx_header.StdId;
        can2_rx_cnt++;
        motor_id = (uint8_t)(rx_data[0] - 1U + 5U);
        if ((motor_id >= 5U) && (motor_id < 10U)) {
          data_can_mit_anal(&motor_chassis[motor_id], rx_data);
          leg_motor.q_now[motor_id] = motor_chassis[motor_id].q_now_flt;
          leg_motor.qd_now[motor_id] = motor_chassis[motor_id].qd_now_flt;
          leg_motor.t_now[motor_id] = motor_chassis[motor_id].t_now_flt;
        }
      }
    }
  }
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
  can_rx_handler(hcan);
}

static uint32_t can_mode_from_legacy(uint8_t mode)
{
  switch (mode) {
    case 1U:
      return CAN_MODE_LOOPBACK;
    case 2U:
      return CAN_MODE_SILENT;
    case 3U:
      return CAN_MODE_SILENT_LOOPBACK;
    default:
      return CAN_MODE_NORMAL;
  }
}

static uint32_t can_sjw_from_legacy(uint8_t tsjw)
{
  switch (tsjw) {
    case 2U:
      return CAN_SJW_2TQ;
    case 3U:
      return CAN_SJW_3TQ;
    case 4U:
      return CAN_SJW_4TQ;
    default:
      return CAN_SJW_1TQ;
  }
}

static uint32_t can_bs1_from_legacy(uint8_t tbs1)
{
  switch (tbs1) {
    case 2U: return CAN_BS1_2TQ;
    case 3U: return CAN_BS1_3TQ;
    case 4U: return CAN_BS1_4TQ;
    case 5U: return CAN_BS1_5TQ;
    case 6U: return CAN_BS1_6TQ;
    case 7U: return CAN_BS1_7TQ;
    case 8U: return CAN_BS1_8TQ;
    case 9U: return CAN_BS1_9TQ;
    case 10U: return CAN_BS1_10TQ;
    case 11U: return CAN_BS1_11TQ;
    case 12U: return CAN_BS1_12TQ;
    case 13U: return CAN_BS1_13TQ;
    case 14U: return CAN_BS1_14TQ;
    case 15U: return CAN_BS1_15TQ;
    case 16U: return CAN_BS1_16TQ;
    default: return CAN_BS1_1TQ;
  }
}

static uint32_t can_bs2_from_legacy(uint8_t tbs2)
{
  switch (tbs2) {
    case 2U: return CAN_BS2_2TQ;
    case 3U: return CAN_BS2_3TQ;
    case 4U: return CAN_BS2_4TQ;
    case 5U: return CAN_BS2_5TQ;
    case 6U: return CAN_BS2_6TQ;
    case 7U: return CAN_BS2_7TQ;
    case 8U: return CAN_BS2_8TQ;
    default: return CAN_BS2_1TQ;
  }
}

static uint32_t can_prescaler_from_legacy(float brp)
{
  uint32_t prescaler = (uint32_t)(brp + 0.5f);
  if (prescaler == 0U) {
    prescaler = 1U;
  }
  if (prescaler > 1024U) {
    prescaler = 1024U;
  }
  return prescaler;
}

static HAL_StatusTypeDef can_common_start(CAN_HandleTypeDef *hcan, uint32_t filter_bank)
{
  CAN_FilterTypeDef filter = {0};

  filter.FilterBank = filter_bank;
  filter.FilterMode = CAN_FILTERMODE_IDMASK;
  filter.FilterScale = CAN_FILTERSCALE_32BIT;
  filter.FilterIdHigh = 0x0000U;
  filter.FilterIdLow = 0x0000U;
  filter.FilterMaskIdHigh = 0x0000U;
  filter.FilterMaskIdLow = 0x0000U;
  filter.FilterFIFOAssignment = CAN_RX_FIFO0;
  filter.FilterActivation = ENABLE;
  filter.SlaveStartFilterBank = 14U;

  if (HAL_CAN_ConfigFilter(hcan, &filter) != HAL_OK) {
    return HAL_ERROR;
  }

  if (HAL_CAN_ActivateNotification(hcan, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK) {
    return HAL_ERROR;
  }

  if (HAL_CAN_Start(hcan) != HAL_OK) {
    return HAL_ERROR;
  }

  return HAL_OK;
}

static HAL_StatusTypeDef can_init_hw(CAN_HandleTypeDef *hcan,
                                     CAN_TypeDef *instance,
                                     uint8_t tsjw,
                                     uint8_t tbs2,
                                     uint8_t tbs1,
                                     float brp,
                                     uint8_t mode)
{
  hcan->Instance = instance;
  hcan->Init.Prescaler = can_prescaler_from_legacy(brp);
  hcan->Init.Mode = can_mode_from_legacy(mode);
  hcan->Init.SyncJumpWidth = can_sjw_from_legacy(tsjw);
  hcan->Init.TimeSeg1 = can_bs1_from_legacy(tbs1);
  hcan->Init.TimeSeg2 = can_bs2_from_legacy(tbs2);
  hcan->Init.TimeTriggeredMode = DISABLE;
  hcan->Init.AutoBusOff = ENABLE;
  hcan->Init.AutoWakeUp = ENABLE;
  hcan->Init.AutoRetransmission = ENABLE;
  hcan->Init.ReceiveFifoLocked = DISABLE;
  hcan->Init.TransmitFifoPriority = DISABLE;

  return HAL_CAN_Init(hcan);
}

uint8_t CAN1_Mode_Init(uint8_t tsjw, uint8_t tbs2, uint8_t tbs1, float brp, uint8_t mode)
{
  if (can_init_hw(&hcan1, CAN1, tsjw, tbs2, tbs1, brp, mode) != HAL_OK) {
    return 1U;
  }

  if (can_common_start(&hcan1, 0U) != HAL_OK) {
    return 1U;
  }

  return 0U;
}

uint8_t CAN2_Mode_Init(uint8_t tsjw, uint8_t tbs2, uint8_t tbs1, float brp, uint8_t mode)
{
  if (can_init_hw(&hcan2, CAN2, tsjw, tbs2, tbs1, brp, mode) != HAL_OK) {
    return 1U;
  }

  if (can_common_start(&hcan2, 14U) != HAL_OK) {
    return 1U;
  }

  return 0U;
}

static uint8_t can_send_msg(CAN_HandleTypeDef *hcan, uint8_t *msg, uint8_t len, uint32_t id)
{
  uint32_t mailbox = 0U;
  CAN_TxHeaderTypeDef tx = {0};

  if ((msg == NULL) || (len > 8U)) {
    return 1U;
  }

  tx.StdId = id & 0x7FFU;
  tx.ExtId = 0U;
  tx.IDE = CAN_ID_STD;
  tx.RTR = CAN_RTR_DATA;
  tx.DLC = len;
  tx.TransmitGlobalTime = DISABLE;

  if (HAL_CAN_AddTxMessage(hcan, &tx, msg, &mailbox) != HAL_OK) {
    return 1U;
  }

  return 0U;
}

uint8_t CAN1_Send_Msg(uint8_t *msg, uint8_t len, uint32_t id)
{
  return can_send_msg(&hcan1, msg, len, id);
}

uint8_t CAN2_Send_Msg(uint8_t *msg, uint8_t len, uint32_t id)
{
  return can_send_msg(&hcan2, msg, len, id);
}

static uint8_t can_receive_msg(CAN_HandleTypeDef *hcan, uint8_t *buf, uint32_t *rx_id, int *rx_cnt)
{
  CAN_RxHeaderTypeDef rx = {0};

  if (buf == NULL) {
    return 0U;
  }

  if (HAL_CAN_GetRxFifoFillLevel(hcan, CAN_RX_FIFO0) == 0U) {
    return 0U;
  }

  if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx, buf) != HAL_OK) {
    return 0U;
  }

  *rx_id = rx.StdId;
  (*rx_cnt)++;

  return (uint8_t)rx.DLC;
}

uint8_t CAN1_Receive_Msg(uint8_t *buf)
{
  return can_receive_msg(&hcan1, buf, &can1_rx_id, &can1_rx_cnt);
}

uint8_t CAN2_Receive_Msg(uint8_t *buf)
{
  return can_receive_msg(&hcan2, buf, &can2_rx_id, &can2_rx_cnt);
}

uint8_t CAN_Send_MIT(motor_measure_t *ptr)
{
  uint8_t data[8];
  int inv_q_flag = ptr->param.q_flag ? 1 : -1;

  float set_q = inv_q_flag * ptr->set_q;
  float set_dq = inv_q_flag * ptr->set_qd;
  float set_t = inv_q_flag * ptr->set_t;

  float p_des = LIMIT(set_q, P_MIN_CAN_MIT[ptr->param.id], P_MAX_CAN_MIT[ptr->param.id]);
  float v_des = LIMIT(set_dq, V_MIN_CAN_MIT[ptr->param.id], V_MAX_CAN_MIT[ptr->param.id]);
  float kp = LIMIT(ptr->kp, KP_MIN_CAN_MIT[ptr->param.id], KP_MAX_CAN_MIT[ptr->param.id]);
  float kd = LIMIT(ptr->kd, KD_MIN_CAN_MIT[ptr->param.id], KD_MAX_CAN_MIT[ptr->param.id]);
  float t_ff = LIMIT(set_t, T_MIN_CAN_MIT[ptr->param.id], T_MAX_CAN_MIT[ptr->param.id]);

  uint16_t p_int = float_to_uint_mit(p_des, P_MIN_CAN_MIT[ptr->param.id], P_MAX_CAN_MIT[ptr->param.id], 16);
  uint16_t v_int = float_to_uint_mit(v_des, V_MIN_CAN_MIT[ptr->param.id], V_MAX_CAN_MIT[ptr->param.id], 12);
  uint16_t kp_int = float_to_uint_mit(kp, KP_MIN_CAN_MIT[ptr->param.id], KP_MAX_CAN_MIT[ptr->param.id], 12);
  uint16_t kd_int = float_to_uint_mit(kd, KD_MIN_CAN_MIT[ptr->param.id], KD_MAX_CAN_MIT[ptr->param.id], 12);
  uint16_t t_int = float_to_uint_mit(t_ff, T_MIN_CAN_MIT[ptr->param.id], T_MAX_CAN_MIT[ptr->param.id], 12);

  data[0] = p_int >> 8;
  data[1] = p_int & 0xFF;
  data[2] = v_int >> 4;
  data[3] = ((v_int & 0xFU) << 4) | (kp_int >> 8);
  data[4] = kp_int & 0xFF;
  data[5] = kd_int >> 4;
  data[6] = ((kd_int & 0xFU) << 4) | (t_int >> 8);
  data[7] = t_int & 0xFFU;

  if (ptr->param.id < 5) {
    return CAN1_Send_Msg(data, 8, (uint32_t)ptr->param.id + 1U);
  }

  return CAN2_Send_Msg(data, 8, (uint32_t)ptr->param.id + 1U - 5U);
}

char data_can_mit_send(motor_measure_t *ptr)
{
  return (char)CAN_Send_MIT(ptr);
}

uint8_t CAN_MIT_Motor_Mode_En(uint8_t id, uint8_t en)
{
  uint8_t data[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00};
  data[7] = en ? 0xFC : 0xFD;

  if (id < 5U) {
    return CAN1_Send_Msg(data, 8, id + 1U);
  }

  return CAN2_Send_Msg(data, 8, id + 1U - 5U);
}

uint8_t mit_motor_mode_en(uint8_t id, uint8_t en)
{
  return CAN_MIT_Motor_Mode_En(id, en);
}

uint8_t mit_set_pos_zero(uint8_t id)
{
  uint8_t data[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE};

  if (id < 5U) {
    return CAN1_Send_Msg(data, 8, id + 1U);
  }

  return CAN2_Send_Msg(data, 8, id + 1U - 5U);
}

void CAN_motor_sm(float dt)
{
  uint8_t any_connected = 0U;

  for (uint8_t i = 0U; i < NUM_MOTORS; i++) {
    motor_measure_t *motor = &motor_chassis[i];

    if (motor->param.connect) {
      motor->param.loss_cnt++;
      if ((uint32_t)motor->param.loss_cnt > CAN_TIMEOUT_MS) {
        motor->param.connect = 0;
        motor->param.loss_cnt = (int)CAN_TIMEOUT_MS;
      }
    }

    if (motor->param.connect) {
      any_connected = 1U;
      leg_motor.connect_motor[i] = 1;
      leg_motor.ready[i] = leg_motor.motor_en ? 1 : 0;
    } else {
      leg_motor.connect_motor[i] = 0;
      leg_motor.ready[i] = 0;
    }

    if ((motor->reset_q == 1) && (motor->reset_q_lock == 0)) {
      if (leg_motor.motor_en == 0) {
        (void)mit_set_pos_zero(i);
        motor->reset_q = 0;
        motor->reset_q_cnt = 0;
        motor->reset_q_lock = 1;
        motor->reset_q_delay_timer = 0.0f;
      }
    } else if (motor->reset_q_lock == 1) {
      (void)mit_set_pos_zero(i);
      motor->reset_q_cnt++;
      if (motor->reset_q_cnt >= (char)RESET_Q_REPEAT_COUNT) {
        motor->reset_q_cnt = 0;
        motor->reset_q_lock = 2;
      }
    } else if (motor->reset_q_lock == 2) {
      motor->reset_q_delay_timer += dt;
      if (motor->reset_q_delay_timer > RESET_Q_SETTLE_TIME_S) {
        motor->reset_q_delay_timer = 0.0f;
        motor->reset_q_lock = 0;
        motor->param.q_reset_angle = 0.0f;
      }
    }
  }

  leg_motor.connect = (char)any_connected;
}

void can_init(void)
{
  can_init_motor_map();

  /* APB1 = 42 MHz, 1 Mbps nominal: Prescaler=3, BS1=11TQ, BS2=2TQ, SJW=1TQ */
  (void)CAN1_Mode_Init(1U, 2U, 11U, 3.0f, 0U);
  (void)CAN2_Mode_Init(1U, 2U, 11U, 3.0f, 0U);
}

void HAL_CAN_MspInit(CAN_HandleTypeDef *hcan)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  if (hcan->Instance == CAN1) {
    __HAL_RCC_CAN1_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitStruct.Pin = CAN1_RX_PIN | CAN1_TX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_CAN1;
    HAL_GPIO_Init(CAN1_RX_PORT, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(CAN1_RX0_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);
  } else if (hcan->Instance == CAN2) {
    __HAL_RCC_CAN1_CLK_ENABLE();
    __HAL_RCC_CAN2_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitStruct.Pin = CAN2_RX_PIN | CAN2_TX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_CAN2;
    HAL_GPIO_Init(CAN2_RX_PORT, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(CAN2_RX0_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(CAN2_RX0_IRQn);
  }
}

void HAL_CAN_MspDeInit(CAN_HandleTypeDef *hcan)
{
  if (hcan->Instance == CAN1) {
    HAL_GPIO_DeInit(CAN1_RX_PORT, CAN1_RX_PIN | CAN1_TX_PIN);
    __HAL_RCC_CAN1_CLK_DISABLE();
  } else if (hcan->Instance == CAN2) {
    HAL_GPIO_DeInit(CAN2_RX_PORT, CAN2_RX_PIN | CAN2_TX_PIN);
    __HAL_RCC_CAN2_CLK_DISABLE();
  }
}
