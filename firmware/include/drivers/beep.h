#ifndef BEEP_H
#define BEEP_H

#include <stdint.h>
#include "stm32f4xx_hal.h"

/* Legacy melody IDs */
#define MEMS_RIGHT_BEEP                  0U
#define MEMS_ERROR_BEEP                  1U
#define START_BEEP                       2U
#define BAT_ERO_BEEP                     3U
#define RC_ERO_BEEP                      4U
#define BEEP_ONE                         5U
#define BEEP_TWO                         6U
#define BEEP_THREE                       7U
#define BEEP_STATE                       8U
#define BEEP_GPS_SAVE                    9U
#define BEEP_HML_CAL                     10U
#define BEEP_MISSION                     11U
#define MEMS_WAY_UPDATE                  12U
#define MEMS_GPS_RIGHT                   14U
#define BEEP_DJ_CAL1                     15U
#define BEEP_DJ_CAL2                     16U
#define BEEP_BLDC_ZERO_CAL               17U
#define BEEP_BLDC_ZERO_INIT              18U
#define BEEP_BLDC_GAIT_SWITCH            19U
#define BEEP_BLDC_STATE                  20U
#define BEEP_BLDC_RESET_ERR              21U
#define BEEP_BLDC_SPI_CONNECT            22U
#define BEEP_BLDC_SPI_CONNECT_THREAD_UP  23U

void Beep_Init(uint32_t arr, uint32_t psc);
HAL_StatusTypeDef beep_service_start(void);
HAL_StatusTypeDef beep_post(uint8_t melodyId);
uint32_t beep_queue_free_slots(void);

#endif /* BEEP_H */
