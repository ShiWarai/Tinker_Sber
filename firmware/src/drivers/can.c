#include "drivers/can.h"
#include "drivers/mit_protocol.h"
#include "system/board_pins.h"
#include "cmsis_os2.h"
#include <string.h>

/* ---------- configuration constants ---------- */

#define CAN_TX_QUEUE_LEN              40U
#define CAN_TOTAL_MOTORS              10U
#define CAN_DEGRADED_TIMEOUT_MS       100U
#define CAN_SAFE_STOP_TIMEOUT_MS      300U
#define CAN_SAFE_STOP_SEND_PERIOD_MS  20U
#define CAN_SAFE_STOP_TORQUE_STEP     0.5f
#define CAN_MIT_CMD_ENABLE            0xFCU
#define CAN_MIT_CMD_DISABLE           0xFDU
#define CAN_MIT_CMD_SET_ZERO          0xFEU
#define CAN_MODE_FRAME_PERIOD_MS      20U
#define CAN_MODE_STAGE1_HOLD_MS       500U
#define CAN_MODE_DISABLED_PING_MS     500U
#define CAN_SAFE_TORQUE_INIT          10.0f
#define CAN_TX_DRAIN_PER_STEP         6U

/* ---------- private types ---------- */

typedef struct {
  can_bus_t  bus;
  uint16_t   std_id;
  uint8_t    data[8];
} can_tx_item_t;

typedef struct {
  uint8_t          motor_id;
  can_bus_t        bus;
  uint16_t         std_id;
  can_motor_type_t type;
  uint8_t          invert_direction;
} can_motor_cfg_t;

/* ---------- HAL handles ---------- */

static CAN_HandleTypeDef hcan1;
static CAN_HandleTypeDef hcan2;

/* ---------- volatile diagnostic variables (debugger watch) ---------- */

volatile can_motor_feedback_t can1_feedback_table[CAN1_FEEDBACK_MAX_MOTORS];
volatile can_motor_feedback_t can1_feedback_last;

volatile uint32_t can1_rx_any_count;
volatile uint32_t can1_rx_mapped_count;
volatile uint32_t can1_rx_unmapped_count;
volatile uint32_t can1_rx_short_count;

volatile uint16_t can1_last_rx_std_id;
volatile uint8_t  can1_last_rx_dlc;
volatile uint8_t  can1_last_rx_bus;
volatile uint8_t  can1_last_rx_local_id;
volatile uint8_t  can1_last_rx_data[8];

volatile uint32_t can1_irq_rx0_count;
volatile uint32_t can2_irq_rx0_count;

volatile uint32_t can1_last_esr_can1;
volatile uint32_t can1_last_esr_can2;

volatile uint8_t  can1_debug_req_enable_all;
volatile uint8_t  can1_debug_req_disable_all;
volatile uint32_t can1_debug_action_count;
volatile uint8_t  can1_enable_request;
volatile uint8_t  can1_mode_state;

/* ---------- private state ---------- */

static volatile uint32_t can1_ready;
static volatile uint32_t can1_rx_count;
static volatile uint32_t can1_err_count;
static volatile uint32_t can1_tx_count;
static volatile uint32_t can1_tx_fail_count;
static volatile uint32_t can1_queue_drop_count;
static volatile uint32_t can1_feedback_update_count;

static osMessageQueueId_t can1_tx_queue;
static can_link_state_t   can1_state;

static uint32_t can1_last_command_ms;
static uint32_t can1_last_safe_tx_ms;
static float    can1_safe_torque;
static uint32_t can1_mode_timer_ms;
static uint32_t can1_mode_last_tx_ms;

/* ---------- motor tables ---------- */

static const mit_limits_t MIT_LIMITS_DM6006 = {
  .p_min  = -12.5f, .p_max  = 12.5f,
  .v_min  = -45.0f, .v_max  = 45.0f,
  .kp_min =   0.0f, .kp_max = 500.0f,
  .kd_min =   0.0f, .kd_max = 5.0f,
  .t_min  = -12.0f, .t_max  = 12.0f,
};

static const mit_limits_t MIT_LIMITS_DM8006 = {
  .p_min  = -12.5f, .p_max  = 12.5f,
  .v_min  = -45.0f, .v_max  = 45.0f,
  .kp_min =   0.0f, .kp_max = 500.0f,
  .kd_min =   0.0f, .kd_max = 5.0f,
  .t_min  = -20.0f, .t_max  = 20.0f,
};

static const can_motor_cfg_t k_motor_cfg[CAN_TOTAL_MOTORS] = {
  { 1,  CAN_BUS_1, 1, CAN_MOTOR_DM6006, 0},
  { 2,  CAN_BUS_1, 2, CAN_MOTOR_DM8006, 1},
  { 3,  CAN_BUS_1, 3, CAN_MOTOR_DM8006, 1},
  { 4,  CAN_BUS_1, 4, CAN_MOTOR_DM8006, 0},
  { 5,  CAN_BUS_1, 5, CAN_MOTOR_DM6006, 1},
  { 6,  CAN_BUS_2, 1, CAN_MOTOR_DM6006, 0},
  { 7,  CAN_BUS_2, 2, CAN_MOTOR_DM8006, 1},
  { 8,  CAN_BUS_2, 3, CAN_MOTOR_DM8006, 0},
  { 9,  CAN_BUS_2, 4, CAN_MOTOR_DM8006, 1},
  {10,  CAN_BUS_2, 5, CAN_MOTOR_DM6006, 0},
};

/* ---------- private helpers ---------- */

static const mit_limits_t *can_limits_for_type(can_motor_type_t type)
{
  return (type == CAN_MOTOR_DM8006) ? &MIT_LIMITS_DM8006 : &MIT_LIMITS_DM6006;
}

static const can_motor_cfg_t *can_cfg_for_motor(uint8_t motor_id)
{
  for (uint32_t i = 0; i < CAN_TOTAL_MOTORS; i++) {
    if (k_motor_cfg[i].motor_id == motor_id) {
      return &k_motor_cfg[i];
    }
  }
  return NULL;
}

static const can_motor_cfg_t *can_cfg_for_rx(can_bus_t bus, uint8_t local_id)
{
  for (uint32_t i = 0; i < CAN_TOTAL_MOTORS; i++) {
    if (k_motor_cfg[i].bus == bus && k_motor_cfg[i].std_id == local_id) {
      return &k_motor_cfg[i];
    }
  }
  return NULL;
}

static CAN_HandleTypeDef *can_handle_for_bus(can_bus_t bus)
{
  return (bus == CAN_BUS_2) ? &hcan2 : &hcan1;
}

static can_bus_t can_bus_from_handle(const CAN_HandleTypeDef *hcan)
{
  return (hcan->Instance == CAN2) ? CAN_BUS_2 : CAN_BUS_1;
}

static uint8_t can_normalize_motor_id(uint8_t id)
{
  return (id >= 0x10U) ? (uint8_t)(id - 0x10U) : id;
}

/* ---------- low-level TX ---------- */

static HAL_StatusTypeDef can_send_std_data(can_bus_t bus, uint16_t std_id,
                                           const uint8_t data[8])
{
  CAN_TxHeaderTypeDef hdr = {
    .StdId              = std_id,
    .ExtId              = 0,
    .IDE                = CAN_ID_STD,
    .RTR                = CAN_RTR_DATA,
    .DLC                = 8,
    .TransmitGlobalTime = DISABLE,
  };
  uint32_t mailbox;

  if (HAL_CAN_AddTxMessage(can_handle_for_bus(bus), &hdr,
                            (uint8_t *)data, &mailbox) != HAL_OK) {
    can1_tx_fail_count++;
    return HAL_ERROR;
  }
  can1_tx_count++;
  return HAL_OK;
}

static HAL_StatusTypeDef can_queue_raw(can_bus_t bus, uint16_t std_id,
                                       const uint8_t data[8])
{
  if (can1_tx_queue == NULL) {
    return can_send_std_data(bus, std_id, data);
  }

  can_tx_item_t item;
  item.bus    = bus;
  item.std_id = std_id;
  memcpy(item.data, data, 8);

  if (osMessageQueuePut(can1_tx_queue, &item, 0U, 0U) != osOK) {
    can1_queue_drop_count++;
    return HAL_BUSY;
  }
  return HAL_OK;
}

/* ---------- internal frame builders ---------- */

static void can_build_mode_frame(uint8_t frame[8], uint8_t enable)
{
  memset(frame, 0xFF, 7);
  frame[7] = enable ? CAN_MIT_CMD_ENABLE : CAN_MIT_CMD_DISABLE;
}

static void can_queue_mode_all(uint8_t enable)
{
  uint8_t frame[8];
  can_build_mode_frame(frame, enable);

  for (uint32_t i = 0; i < CAN_TOTAL_MOTORS; i++) {
    (void)can_queue_raw(k_motor_cfg[i].bus, k_motor_cfg[i].std_id, frame);
  }
}

static void can_queue_safe_stop_once(void)
{
  if (can1_safe_torque > 0.0f) {
    can1_safe_torque -= CAN_SAFE_STOP_TORQUE_STEP;
    if (can1_safe_torque < 0.0f) {
      can1_safe_torque = 0.0f;
    }
  }

  mit_command_t cmd = {0};
  cmd.t = can1_safe_torque;
  uint8_t frame[8];

  for (uint32_t i = 0; i < CAN_TOTAL_MOTORS; i++) {
    mit_pack_command(&cmd, can_limits_for_type(k_motor_cfg[i].type), frame);
    (void)can_queue_raw(k_motor_cfg[i].bus, k_motor_cfg[i].std_id, frame);
  }
}

/* ---------- mode state machine ---------- */

static void can_process_debug_requests(void)
{
  if (can1_debug_req_enable_all != 0U) {
    can1_debug_req_enable_all = 0U;
    can1_enable_request = 1U;
    can1_debug_action_count++;
  }
  if (can1_debug_req_disable_all != 0U) {
    can1_debug_req_disable_all = 0U;
    can1_enable_request = 0U;
    can1_debug_action_count++;
  }
}

static void can_process_mode_state_1ms(void)
{
  uint32_t now = HAL_GetTick();

  switch (can1_mode_state) {
    case 0U:
      if (can1_enable_request != 0U) {
        can_queue_mode_all(1U);
        can1_mode_state      = 1U;
        can1_mode_timer_ms   = now;
        can1_mode_last_tx_ms = now;
      } else if ((now - can1_mode_last_tx_ms) >= CAN_MODE_DISABLED_PING_MS) {
        can_queue_mode_all(0U);
        can1_mode_last_tx_ms = now;
      }
      break;

    case 1U:
      if ((now - can1_mode_last_tx_ms) >= CAN_MODE_FRAME_PERIOD_MS) {
        can_queue_mode_all(1U);
        can1_mode_last_tx_ms = now;
      }
      if ((now - can1_mode_timer_ms) >= CAN_MODE_STAGE1_HOLD_MS) {
        can1_mode_state = 2U;
      }
      break;

    case 2U:
      if (can1_enable_request == 0U) {
        can_queue_mode_all(0U);
        can1_mode_state      = 0U;
        can1_mode_last_tx_ms = now;
      }
      break;

    default:
      can1_mode_state      = 0U;
      can1_mode_timer_ms   = now;
      can1_mode_last_tx_ms = now;
      break;
  }
}

/* ---------- feedback storage ---------- */

static void can_store_feedback(const can_motor_cfg_t *cfg,
                               const mit_feedback_t *fb)
{
  if (cfg == NULL || fb == NULL) {
    return;
  }

  uint8_t slot = cfg->motor_id;
  if (slot == 0U || slot > CAN1_FEEDBACK_MAX_MOTORS) {
    return;
  }
  slot--;

  can1_feedback_table[slot].valid      = 1U;
  can1_feedback_table[slot].motor_id   = cfg->motor_id;
  can1_feedback_table[slot].bus        = (uint8_t)cfg->bus;
  can1_feedback_table[slot].std_id     = cfg->std_id;
  can1_feedback_table[slot].rx_tick_ms = HAL_GetTick();
  can1_feedback_table[slot].p = cfg->invert_direction ? -fb->p : fb->p;
  can1_feedback_table[slot].v = cfg->invert_direction ? -fb->v : fb->v;
  can1_feedback_table[slot].t = cfg->invert_direction ? -fb->t : fb->t;

  can1_feedback_last = can1_feedback_table[slot];
  can1_feedback_update_count++;
}

/* ---------- HAL CAN hardware init (low-level) ---------- */

static HAL_StatusTypeDef can_filter_configure(CAN_HandleTypeDef *hcan,
                                              uint32_t filter_bank)
{
  CAN_FilterTypeDef filter = {
    .FilterBank           = filter_bank,
    .FilterMode           = CAN_FILTERMODE_IDMASK,
    .FilterScale          = CAN_FILTERSCALE_32BIT,
    .FilterIdHigh         = 0,
    .FilterIdLow          = 0,
    .FilterMaskIdHigh     = 0,
    .FilterMaskIdLow      = 0,
    .FilterFIFOAssignment = CAN_FILTER_FIFO0,
    .FilterActivation     = ENABLE,
    .SlaveStartFilterBank = 14,
  };
  return HAL_CAN_ConfigFilter(hcan, &filter);
}

static HAL_StatusTypeDef can_handle_init(CAN_HandleTypeDef *hcan,
                                         CAN_TypeDef *instance)
{
  hcan->Instance                  = instance;
  hcan->Init.Prescaler            = 3;
  hcan->Init.Mode                 = CAN_MODE_NORMAL;
  hcan->Init.SyncJumpWidth        = CAN_SJW_2TQ;
  hcan->Init.TimeSeg1             = CAN_BS1_9TQ;
  hcan->Init.TimeSeg2             = CAN_BS2_4TQ;
  hcan->Init.TimeTriggeredMode    = DISABLE;
  hcan->Init.AutoBusOff           = ENABLE;
  hcan->Init.AutoWakeUp           = ENABLE;
  hcan->Init.AutoRetransmission   = ENABLE;
  hcan->Init.ReceiveFifoLocked    = DISABLE;
  hcan->Init.TransmitFifoPriority = DISABLE;

  if (HAL_CAN_Init(hcan) != HAL_OK) {
    return HAL_ERROR;
  }
  if (can_filter_configure(hcan, (instance == CAN1) ? 0U : 14U) != HAL_OK) {
    return HAL_ERROR;
  }
  if (HAL_CAN_Start(hcan) != HAL_OK) {
    return HAL_ERROR;
  }
  if (HAL_CAN_ActivateNotification(hcan,
                                    CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK) {
    return HAL_ERROR;
  }
  return HAL_OK;
}

/* ================================================================== */
/*  PUBLIC API                                                        */
/* ================================================================== */

HAL_StatusTypeDef can1_init(void)
{
  memset((void *)can1_feedback_table, 0, sizeof(can1_feedback_table));
  memset((void *)&can1_feedback_last, 0, sizeof(can1_feedback_last));
  memset((void *)can1_last_rx_data,   0, sizeof(can1_last_rx_data));

  for (uint32_t i = 0; i < CAN1_FEEDBACK_MAX_MOTORS; i++) {
    can1_feedback_table[i].motor_id = (uint8_t)(i + 1U);
  }

  can1_ready               = 0U;
  can1_rx_count            = 0U;
  can1_rx_any_count        = 0U;
  can1_rx_mapped_count     = 0U;
  can1_rx_unmapped_count   = 0U;
  can1_rx_short_count      = 0U;
  can1_last_rx_std_id      = 0U;
  can1_last_rx_dlc         = 0U;
  can1_last_rx_bus         = 0U;
  can1_last_rx_local_id    = 0U;
  can1_irq_rx0_count       = 0U;
  can2_irq_rx0_count       = 0U;
  can1_last_esr_can1       = 0U;
  can1_last_esr_can2       = 0U;
  can1_err_count           = 0U;
  can1_tx_count            = 0U;
  can1_tx_fail_count       = 0U;
  can1_queue_drop_count    = 0U;
  can1_feedback_update_count = 0U;
  can1_debug_req_enable_all  = 0U;
  can1_debug_req_disable_all = 0U;
  can1_debug_action_count    = 0U;
  can1_enable_request        = 0U;
  can1_mode_state            = 0U;

  can1_tx_queue        = NULL;
  can1_state           = CAN_LINK_DEGRADED;
  can1_last_command_ms = HAL_GetTick();
  can1_last_safe_tx_ms = 0U;
  can1_safe_torque     = CAN_SAFE_TORQUE_INIT;
  can1_mode_timer_ms   = can1_last_command_ms;
  can1_mode_last_tx_ms = can1_last_command_ms;

  if (can_handle_init(&hcan1, CAN1) != HAL_OK) {
    return HAL_ERROR;
  }
  if (can_handle_init(&hcan2, CAN2) != HAL_OK) {
    return HAL_ERROR;
  }

  can1_ready = 1U;
  return HAL_OK;
}

HAL_StatusTypeDef can1_service_start(void)
{
  if (!can1_is_ready()) {
    return HAL_ERROR;
  }

  if (can1_tx_queue == NULL) {
    can1_tx_queue = osMessageQueueNew(CAN_TX_QUEUE_LEN,
                                       sizeof(can_tx_item_t), NULL);
    if (can1_tx_queue == NULL) {
      return HAL_ERROR;
    }
  }

  uint32_t now = HAL_GetTick();
  can1_last_command_ms = now;
  can1_state           = CAN_LINK_DEGRADED;
  can1_safe_torque     = CAN_SAFE_TORQUE_INIT;
  can1_mode_state      = 0U;
  can1_mode_timer_ms   = now;
  can1_mode_last_tx_ms = now;
  return HAL_OK;
}

void can1_service_step_1ms(void)
{
  if (can1_tx_queue == NULL || !can1_is_ready()) {
    return;
  }

  can1_last_esr_can1 = CAN1->ESR;
  can1_last_esr_can2 = CAN2->ESR;

  can_process_debug_requests();
  can_process_mode_state_1ms();

  uint32_t now        = HAL_GetTick();
  uint32_t since_last = now - can1_last_command_ms;

  if (since_last >= CAN_SAFE_STOP_TIMEOUT_MS) {
    can1_state = CAN_LINK_SAFE_STOP;
  } else if (since_last >= CAN_DEGRADED_TIMEOUT_MS) {
    can1_state = CAN_LINK_DEGRADED;
  } else {
    can1_state = CAN_LINK_RUN;
  }

  if (can1_state == CAN_LINK_SAFE_STOP &&
      (now - can1_last_safe_tx_ms) >= CAN_SAFE_STOP_SEND_PERIOD_MS) {
    can1_last_safe_tx_ms = now;
    can_queue_safe_stop_once();
  }

  can_tx_item_t item;
  for (uint32_t sent = 0; sent < CAN_TX_DRAIN_PER_STEP; sent++) {
    if (osMessageQueueGet(can1_tx_queue, &item, NULL, 0U) != osOK) {
      break;
    }
    (void)can_send_std_data(item.bus, item.std_id, item.data);
  }
}

/* ---- motor control ---- */

void can1_set_enable_request(uint8_t enable)
{
  can1_enable_request = (enable != 0U) ? 1U : 0U;
}

HAL_StatusTypeDef can_send_motor_mode(uint8_t motor_id, uint8_t enable)
{
  const can_motor_cfg_t *cfg = can_cfg_for_motor(motor_id);
  if (cfg == NULL) {
    return HAL_ERROR;
  }

  uint8_t frame[8];
  can_build_mode_frame(frame, enable);
  return can_queue_raw(cfg->bus, cfg->std_id, frame);
}

HAL_StatusTypeDef can_send_motor_set_zero(uint8_t motor_id)
{
  const can_motor_cfg_t *cfg = can_cfg_for_motor(motor_id);
  if (cfg == NULL) {
    return HAL_ERROR;
  }

  uint8_t frame[8];
  memset(frame, 0xFF, 7);
  frame[7] = CAN_MIT_CMD_SET_ZERO;
  return can_queue_raw(cfg->bus, cfg->std_id, frame);
}

HAL_StatusTypeDef can1_queue_mit_command(uint16_t std_id,
                                         const mit_command_t *cmd,
                                         const mit_limits_t *limits)
{
  if (cmd == NULL || std_id == 0U || can1_tx_queue == NULL) {
    return HAL_ERROR;
  }

  const mit_limits_t *lim = (limits != NULL) ? limits : &MIT_LIMITS_DM_DEFAULT;

  can_tx_item_t item;
  item.bus    = CAN_BUS_1;
  item.std_id = std_id;
  mit_pack_command(cmd, lim, item.data);

  if (osMessageQueuePut(can1_tx_queue, &item, 0U, 0U) != osOK) {
    can1_queue_drop_count++;
    return HAL_BUSY;
  }

  can1_last_command_ms = HAL_GetTick();
  can1_state           = CAN_LINK_RUN;
  can1_safe_torque     = CAN_SAFE_TORQUE_INIT;
  return HAL_OK;
}

HAL_StatusTypeDef can_queue_motor_command(uint8_t motor_id,
                                          const mit_command_t *cmd)
{
  if (cmd == NULL) {
    return HAL_ERROR;
  }

  const can_motor_cfg_t *cfg = can_cfg_for_motor(motor_id);
  if (cfg == NULL || can1_tx_queue == NULL) {
    return HAL_ERROR;
  }

  mit_command_t mapped = *cmd;
  if (cfg->invert_direction) {
    mapped.p = -mapped.p;
    mapped.v = -mapped.v;
    mapped.t = -mapped.t;
  }

  can_tx_item_t item;
  item.bus    = cfg->bus;
  item.std_id = cfg->std_id;
  mit_pack_command(&mapped, can_limits_for_type(cfg->type), item.data);

  if (osMessageQueuePut(can1_tx_queue, &item, 0U, 0U) != osOK) {
    can1_queue_drop_count++;
    return HAL_BUSY;
  }

  can1_last_command_ms = HAL_GetTick();
  can1_state           = CAN_LINK_RUN;
  can1_safe_torque     = CAN_SAFE_TORQUE_INIT;
  return HAL_OK;
}

/* ---- feedback / status query ---- */

uint8_t can1_is_ready(void)
{
  return (uint8_t)(can1_ready != 0U);
}

can_link_state_t can1_link_state(void)
{
  return can1_state;
}

uint32_t can1_rx_frames(void)       { return can1_rx_count; }
uint32_t can1_error_events(void)    { return can1_err_count; }
uint32_t can1_tx_frames(void)       { return can1_tx_count; }
uint32_t can1_tx_failures(void)     { return can1_tx_fail_count; }
uint32_t can1_queue_drops(void)     { return can1_queue_drop_count; }
uint32_t can1_feedback_updates(void){ return can1_feedback_update_count; }

uint8_t can1_get_feedback(uint8_t motor_id, can_motor_feedback_t *out)
{
  if (motor_id == 0U || motor_id > CAN1_FEEDBACK_MAX_MOTORS || out == NULL) {
    return 0U;
  }

  uint8_t slot = (uint8_t)(motor_id - 1U);
  if (can1_feedback_table[slot].valid == 0U) {
    return 0U;
  }

  *out = can1_feedback_table[slot];
  return 1U;
}

uint8_t can1_get_last_feedback(can_motor_feedback_t *out)
{
  if (out == NULL || can1_feedback_last.valid == 0U) {
    return 0U;
  }

  *out = can1_feedback_last;
  return 1U;
}

uint8_t can1_is_motor_connected(uint8_t motor_id, uint32_t timeout_ms)
{
  if (motor_id == 0U || motor_id > CAN1_FEEDBACK_MAX_MOTORS) {
    return 0U;
  }

  uint8_t slot = (uint8_t)(motor_id - 1U);
  if (can1_feedback_table[slot].valid == 0U) {
    return 0U;
  }

  uint32_t elapsed = HAL_GetTick() - can1_feedback_table[slot].rx_tick_ms;
  return (elapsed <= timeout_ms) ? 1U : 0U;
}

uint8_t can1_connected_count(uint32_t timeout_ms)
{
  uint8_t count = 0;
  uint32_t now = HAL_GetTick();

  for (uint32_t i = 0; i < CAN_TOTAL_MOTORS; i++) {
    uint8_t slot = (uint8_t)(k_motor_cfg[i].motor_id - 1U);
    if (can1_feedback_table[slot].valid != 0U &&
        (now - can1_feedback_table[slot].rx_tick_ms) <= timeout_ms) {
      count++;
    }
  }
  return count;
}

/* ================================================================== */
/*  HAL MSP callbacks                                                 */
/* ================================================================== */

void HAL_CAN_MspInit(CAN_HandleTypeDef *hcan)
{
  GPIO_InitTypeDef gpio = {0};
  __HAL_RCC_GPIOB_CLK_ENABLE();

  if (hcan->Instance == CAN1) {
    __HAL_RCC_CAN1_CLK_ENABLE();
    gpio.Pin       = CAN1_RX_PIN | CAN1_TX_PIN;
    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Pull      = GPIO_PULLUP;
    gpio.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF9_CAN1;
    HAL_GPIO_Init(CAN1_GPIO_PORT, &gpio);
    HAL_NVIC_SetPriority(CAN1_RX0_IRQn, 9, 0);
    HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);
    return;
  }

  if (hcan->Instance == CAN2) {
    __HAL_RCC_CAN1_CLK_ENABLE();
    __HAL_RCC_CAN2_CLK_ENABLE();
    gpio.Pin       = CAN2_RX_PIN | CAN2_TX_PIN;
    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Pull      = GPIO_PULLUP;
    gpio.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF9_CAN2;
    HAL_GPIO_Init(CAN2_GPIO_PORT, &gpio);
    HAL_NVIC_SetPriority(CAN2_RX0_IRQn, 9, 0);
    HAL_NVIC_EnableIRQ(CAN2_RX0_IRQn);
  }
}

void HAL_CAN_MspDeInit(CAN_HandleTypeDef *hcan)
{
  if (hcan->Instance == CAN1) {
    HAL_NVIC_DisableIRQ(CAN1_RX0_IRQn);
    HAL_GPIO_DeInit(CAN1_GPIO_PORT, CAN1_RX_PIN | CAN1_TX_PIN);
    __HAL_RCC_CAN1_CLK_DISABLE();
    return;
  }
  if (hcan->Instance == CAN2) {
    HAL_NVIC_DisableIRQ(CAN2_RX0_IRQn);
    HAL_GPIO_DeInit(CAN2_GPIO_PORT, CAN2_RX_PIN | CAN2_TX_PIN);
    __HAL_RCC_CAN2_CLK_DISABLE();
  }
}

/* ================================================================== */
/*  HAL CAN RX callback                                               */
/* ================================================================== */

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
  CAN_RxHeaderTypeDef header;
  uint8_t data[8];

  if (hcan->Instance != CAN1 && hcan->Instance != CAN2) {
    return;
  }

  can_bus_t bus = can_bus_from_handle(hcan);

  if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &header, data) != HAL_OK) {
    can1_err_count++;
    return;
  }

  can1_rx_any_count++;
  can1_last_rx_std_id   = (uint16_t)header.StdId;
  can1_last_rx_dlc      = header.DLC;
  can1_last_rx_bus      = (uint8_t)bus;
  can1_last_rx_local_id = 0U;

  for (uint8_t i = 0U; i < 8U; i++) {
    can1_last_rx_data[i] = (i < header.DLC) ? data[i] : 0U;
  }

  if (header.DLC >= 6U) {
    uint8_t local_id = can_normalize_motor_id(data[0]);
    can1_last_rx_local_id = local_id;

    const can_motor_cfg_t *cfg = can_cfg_for_rx(bus, local_id);
    if (cfg != NULL) {
      mit_feedback_t fb;
      mit_unpack_feedback_legacy(data, can_limits_for_type(cfg->type), &fb);
      can_store_feedback(cfg, &fb);
      can1_rx_mapped_count++;
    } else {
      can1_rx_unmapped_count++;
    }
  } else {
    can1_rx_short_count++;
  }

  can1_rx_count++;
}

/* ================================================================== */
/*  ISR entry points (called from stm32f4xx_it.c)                    */
/* ================================================================== */

void can1_irq_rx0_handler(void)
{
  can1_irq_rx0_count++;
  HAL_CAN_IRQHandler(&hcan1);
}

void can2_irq_rx0_handler(void)
{
  can2_irq_rx0_count++;
  HAL_CAN_IRQHandler(&hcan2);
}
