#ifndef CAN_H
#define CAN_H

#include <stdint.h>
#include "stm32f4xx_hal.h"
#include "drivers/mit_protocol.h"

#define CAN1_FEEDBACK_MAX_MOTORS 16U

typedef enum {
	CAN_BUS_1 = 1,
	CAN_BUS_2 = 2,
} can_bus_t;

typedef enum {
	CAN_MOTOR_DM6006 = 0,
	CAN_MOTOR_DM8006 = 1,
} can_motor_type_t;

typedef enum {
	CAN_LINK_RUN = 0,
	CAN_LINK_DEGRADED = 1,
	CAN_LINK_SAFE_STOP = 2,
} can_link_state_t;

typedef struct {
	uint8_t valid;
	uint8_t motor_id;
	uint8_t bus;
	uint16_t std_id;
	uint32_t rx_tick_ms;
	float p;
	float v;
	float t;
} can_motor_feedback_t;

extern volatile can_motor_feedback_t can1_feedback_table[CAN1_FEEDBACK_MAX_MOTORS];
extern volatile can_motor_feedback_t can1_feedback_last;
extern volatile uint32_t can1_rx_any_count;
extern volatile uint32_t can1_rx_mapped_count;
extern volatile uint32_t can1_rx_unmapped_count;
extern volatile uint32_t can1_rx_short_count;
extern volatile uint16_t can1_last_rx_std_id;
extern volatile uint8_t can1_last_rx_dlc;
extern volatile uint8_t can1_last_rx_bus;
extern volatile uint8_t can1_last_rx_local_id;
extern volatile uint8_t can1_last_rx_data[8];
extern volatile uint32_t can1_irq_rx0_count;
extern volatile uint32_t can1_irq_sce_count;
extern volatile uint32_t can2_irq_rx0_count;
extern volatile uint32_t can2_irq_sce_count;
extern volatile uint32_t can1_last_hal_error;
extern volatile uint32_t can1_last_esr_can1;
extern volatile uint32_t can1_last_esr_can2;
extern volatile uint8_t can1_debug_req_enable_all;
extern volatile uint8_t can1_debug_req_disable_all;
extern volatile uint32_t can1_debug_action_count;
extern volatile uint8_t can1_enable_request;
extern volatile uint8_t can1_mode_state;

HAL_StatusTypeDef can1_init(void);
uint8_t can1_is_ready(void);
uint32_t can1_rx_frames(void);
uint32_t can1_error_events(void);
uint32_t can1_tx_frames(void);
uint32_t can1_tx_failures(void);
uint32_t can1_queue_drops(void);
can_link_state_t can1_link_state(void);
uint32_t can1_feedback_updates(void);

HAL_StatusTypeDef can1_service_start(void);
void can1_service_step_1ms(void);
void can1_set_enable_request(uint8_t enable);

HAL_StatusTypeDef can1_queue_mit_command(uint16_t std_id,
																				 const mit_command_t *cmd,
																				 const mit_limits_t *limits);
HAL_StatusTypeDef can_queue_motor_command(uint8_t motor_id,
										  const mit_command_t *cmd);
HAL_StatusTypeDef can_send_motor_mode(uint8_t motor_id, uint8_t enable);

uint8_t can1_get_feedback(uint8_t motor_id, can_motor_feedback_t *out);
uint8_t can1_get_last_feedback(can_motor_feedback_t *out);

void can1_irq_rx0_handler(void);
void can1_irq_sce_handler(void);
void can2_irq_rx0_handler(void);
void can2_irq_sce_handler(void);

#endif /* CAN_H */