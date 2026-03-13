#include "drivers/beep.h"

#include "cmsis_os2.h"
#include "stm32f4xx_hal.h"
#include "system/board_pins.h"

static TIM_HandleTypeDef htim4;
static uint8_t beep_initialized;
static uint8_t beep_state_task;
static uint16_t beep_state_task_cnt;
static uint8_t dj_sel;
static uint8_t fc_state_beep[3];
static uint8_t fc_save_gps_beep;
static osMessageQueueId_t beep_queue;
static osThreadId_t beep_task_id;

#define BEEP_QUEUE_LEN 8U
#define BEEP_TASK_DT_MS 10U

static void beep_tone(uint8_t level, uint8_t tone);
static void beep_reset_task(void);
static uint8_t beep_play_music_in_task(uint8_t *music, uint16_t st, uint16_t ed, uint8_t en, float dt);
static char beep_play_music_task(uint8_t sel, float dt);

static uint16_t tone_table[3][8] = {
  {0, 261, 293, 329, 349, 391, 440, 493},
  {0, 523, 587, 659, 698, 783, 880, 987},
  {0, 1046, 1174, 1318, 1396, 1567, 1760, 1975}
};

static uint16_t Beat_delay[7] = {0, 62, 94, 125, 125, 187, 250};

static uint8_t start_music_pi[] = {
  27,01, 00,01, 00,59, 00,59
};

static uint8_t start_music_pi_thread[] = {
  27,01, 00,01, 17,01, 00,01, 00,59, 00,59
};

static uint8_t start_music_windows[] = {
  21,21, 01,11, 21,21, 01,11, 21,21, 01,11,
  11,13, 15,13, 13,11, 01,11, 13,11, 01,11, 13,11, 01,11,
  21,23, 15,41, 01,11, 15,31, 01,11, 15,21, 01,11, 15,11, 01,11
};

static uint8_t start_music_micro[] = {
  21,21, 01,11, 21,21, 01,11, 21,21, 01,11,
  11,13, 15,13, 13,11, 01,11, 13,11, 01,11, 13,11, 01,11,
  21,23, 15,41, 01,11, 15,31, 01,11, 15,21, 01,11, 15,11, 01,11
};

static uint8_t mems_gps_music[] = {15,44, 17,33, 17,33};
static uint8_t mems_right_music[] = {05,64, 00,13, 12,64};
static uint8_t mems_error_music[] = {02,34, 00,32, 02,34, 00,32};
static uint8_t mission_music[] = {22,35, 22,35, 22,65};
static uint8_t mems_wayup_music[] = {15,52, 17,32, 17,32};

static uint8_t bat_error_music[] = {
  07,33, 00,32, 07,33, 00,32, 07,33, 00,32, 07,33, 00,32,
  07,33, 00,32, 07,33, 00,32, 07,33, 00,32, 07,33, 00,32,
  07,33, 00,32, 07,33, 00,32, 07,33, 00,32, 07,33, 00,32,
  07,33, 00,32, 07,33, 00,32, 07,33, 00,32, 07,33, 00,32
};

static uint8_t rc_error_music[] = {
  27,11, 00,11, 27,11, 00,11, 27,11, 00,11, 00,39, 00,39
};

static uint8_t hml_cal_music[] = {
  13,11, 00,11, 13,11, 00,11, 13,11, 00,11, 00,39, 00,39
};

static uint8_t bldc_cal_zero_music[] = {05,64, 00,13, 12,64};
static uint8_t bldc_cal_init_music[] = {03,64, 00,13, 15,34};
static uint8_t bldc_reset_err_music[] = {02,34, 00,32, 02,34, 00,32};
static uint8_t gait_switch_music[] = {15,14, 00,13, 12,14};


static uint8_t dj_cal_music1[] = {27,01, 00,01, 00,39, 00,39};
static uint8_t dj_cal_music2[] = {27,01, 00,01, 27,01, 00,01, 00,39, 00,39};
static uint8_t dj_cal_music3[] = {27,01, 00,01, 27,01, 00,01, 27,01, 00,01, 00,39, 00,39};
static uint8_t dj_cal_music4[] = {27,01, 00,01, 27,01, 00,01, 27,01, 00,01, 27,01, 00,01, 00,39, 00,39};
static uint8_t dj_cal_music5[] = {27,01, 00,01, 27,01, 00,01, 27,01, 00,01, 27,01, 00,01, 27,01, 00,01, 00,39, 00,39};
static uint8_t dj_cal_music6[] = {27,01, 00,01, 27,01, 00,01, 27,01, 00,01, 27,01, 00,01, 27,01, 00,01, 27,01, 00,01, 00,39, 00,39};
static uint8_t dj_cal_music7[] = {27,01, 00,01, 27,01, 00,01, 27,01, 00,01, 27,01, 00,01, 27,01, 00,01, 27,01, 00,01, 27,01, 00,01, 00,39, 00,39};
static uint8_t dj_cal_music8[] = {27,01, 00,01, 27,01, 00,01, 27,01, 00,01, 27,01, 00,01, 27,01, 00,01, 27,01, 00,01, 27,01, 00,01, 27,01, 00,01, 00,39, 00,39};

static uint8_t beep_state[] = {
  00,11, 00,21, 00,11, 00,21, 00,11, 00,21, 00,11, 00,11
};

static uint8_t beep_bldc_state[] = {
  00,11, 00,22, 00,11, 00,22, 00,11, 00,22, 00,11, 00,22, 00,22, 00,22
};

static uint8_t beep_gps_save[] = {
  00,11, 00,21, 00,11, 00,21
};

static void beep_task(void *argument)
{
  uint8_t active;
  uint8_t current = 0U;
  const float dt = ((float)BEEP_TASK_DT_MS) / 1000.0f;

  (void)argument;
  active = 0U;

  for (;;) {
    if (active == 0U) {
      if (osMessageQueueGet(beep_queue, &current, NULL, osWaitForever) == osOK) {
        beep_reset_task();
        active = 1U;
      }
      continue;
    }

    if (beep_play_music_task(current, dt) != 0) {
      active = 0U;
      beep_tone(0U, 0U);
    }

    osDelay(BEEP_TASK_DT_MS);
  }
}

static void beep_set_pwm_hz(uint32_t hz)
{
  uint32_t psc;
  uint32_t arr;

  if (hz == 0U) {
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_2, 0U);
    return;
  }

  /* Legacy tuning: this reproduces the old Tone() pitch/timbre profile. */
  psc = (84U / 2U) - 1U;
  arr = (1000000U / hz) - 1U;

  __HAL_TIM_SET_PRESCALER(&htim4, psc);
  __HAL_TIM_SET_AUTORELOAD(&htim4, arr);
  __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_2, arr / 2U);
  __HAL_TIM_SET_COUNTER(&htim4, 0U);
}

static void beep_tone(uint8_t level, uint8_t tone)
{
  if (!beep_initialized) {
    return;
  }

  if ((level >= 3U) || (tone >= 8U)) {
    beep_set_pwm_hz(0U);
    return;
  }

  beep_set_pwm_hz(tone_table[level][tone]);
}

static void beep_reset_task(void)
{
  beep_tone(0U, 0U);
  beep_state_task = 0U;
  beep_state_task_cnt = 0U;
}

static uint8_t beep_play_music_in_task(uint8_t *music, uint16_t st, uint16_t ed, uint8_t en, float dt)
{
  static uint16_t i;

  switch (beep_state_task) {
    case 0:
      if (en != 0U) {
        beep_state_task = 1U;
        i = st;
        beep_state_task_cnt = 0U;
      }
      break;

    case 1:
      if (en != 0U) {
        if (i >= ed) {
          beep_state_task = 0U;
          beep_tone(0U, 0U);
          return 1U;
        }

        uint8_t level = (uint8_t)(music[i * 2U] / 10U);
        uint8_t tone = (uint8_t)(music[i * 2U] % 10U);
        uint8_t loop = (uint8_t)(music[i * 2U + 1U] % 10U);
        uint8_t beat = (uint8_t)(music[i * 2U + 1U] / 10U);

        if (dt > 0.0f) {
          float ticks = ((float)Beat_delay[beat] * (float)loop) / 1000.0f / dt;
          if ((float)beep_state_task_cnt++ > ticks) {
            beep_tone(level, tone);
            i++;
            beep_state_task_cnt = 0U;
          }
        }
      } else {
        beep_tone(0U, 0U);
        beep_state_task = 0U;
      }
      break;

    default:
      beep_state_task = 0U;
      beep_tone(0U, 0U);
      break;
  }

  return 0U;
}

static char beep_play_music_task(uint8_t sel, float dt)
{
  uint8_t finish = 0U;

  switch (sel) {
    case MEMS_RIGHT_BEEP:
      finish = beep_play_music_in_task(mems_right_music, 0U, (uint16_t)(sizeof(mems_right_music) / 2U), 1U, dt);
      break;

    case START_BEEP:
      finish = beep_play_music_in_task(start_music_windows, 0U, (uint16_t)(sizeof(start_music_windows) / 2U), 1U, dt);
      break;

    case BEEP_BLDC_SPI_CONNECT:
      finish = beep_play_music_in_task(start_music_pi, 0U, (uint16_t)(sizeof(start_music_pi) / 2U), 1U, dt);
      break;

    case BEEP_BLDC_SPI_CONNECT_THREAD_UP:
      finish = beep_play_music_in_task(start_music_pi_thread, 0U, (uint16_t)(sizeof(start_music_pi_thread) / 2U), 1U, dt);
      break;

    case MEMS_GPS_RIGHT:
      finish = beep_play_music_in_task(mems_gps_music, 0U, (uint16_t)(sizeof(mems_gps_music) / 2U), 1U, dt);
      break;

    case MEMS_ERROR_BEEP:
      finish = beep_play_music_in_task(mems_error_music, 0U, (uint16_t)(sizeof(mems_error_music) / 2U), 1U, dt);
      break;

    case MEMS_WAY_UPDATE:
      finish = beep_play_music_in_task(mems_wayup_music, 0U, (uint16_t)(sizeof(mems_wayup_music) / 2U), 1U, dt);
      break;

    case BEEP_MISSION:
      finish = beep_play_music_in_task(mission_music, 0U, (uint16_t)(sizeof(mission_music) / 2U), 1U, dt);
      break;

    case BAT_ERO_BEEP:
      finish = beep_play_music_in_task(bat_error_music, 0U, (uint16_t)(sizeof(bat_error_music) / 2U), 1U, dt);
      break;

    case RC_ERO_BEEP:
      /* Legacy behavior: RC error melody is intentionally muted. */
      beep_tone(0U, 0U);
      finish = 1U;
      break;

    case BEEP_STATE:
      beep_state[0] = fc_state_beep[0];
      beep_state[2] = fc_state_beep[1];
      beep_state[4] = fc_state_beep[2];
      finish = beep_play_music_in_task(beep_state, 0U, (uint16_t)(sizeof(beep_state) / 2U), 1U, dt);
      break;

    case BEEP_GPS_SAVE:
      beep_gps_save[0] = fc_save_gps_beep;
      beep_gps_save[2] = fc_save_gps_beep;
      finish = beep_play_music_in_task(beep_gps_save, 0U, (uint16_t)(sizeof(beep_gps_save) / 2U), 1U, dt);
      break;

    case BEEP_HML_CAL:
      finish = beep_play_music_in_task(hml_cal_music, 0U, (uint16_t)(sizeof(hml_cal_music) / 2U), 1U, dt);
      break;

    case BEEP_DJ_CAL1:
      switch (dj_sel) {
        case 0: finish = beep_play_music_in_task(dj_cal_music1, 0U, (uint16_t)(sizeof(dj_cal_music1) / 2U), 1U, dt); break;
        case 1: finish = beep_play_music_in_task(dj_cal_music2, 0U, (uint16_t)(sizeof(dj_cal_music2) / 2U), 1U, dt); break;
        case 2: finish = beep_play_music_in_task(dj_cal_music3, 0U, (uint16_t)(sizeof(dj_cal_music3) / 2U), 1U, dt); break;
        case 3: finish = beep_play_music_in_task(dj_cal_music4, 0U, (uint16_t)(sizeof(dj_cal_music4) / 2U), 1U, dt); break;
        case 4: finish = beep_play_music_in_task(dj_cal_music5, 0U, (uint16_t)(sizeof(dj_cal_music5) / 2U), 1U, dt); break;
        case 5: finish = beep_play_music_in_task(dj_cal_music6, 0U, (uint16_t)(sizeof(dj_cal_music6) / 2U), 1U, dt); break;
        case 6: finish = beep_play_music_in_task(dj_cal_music7, 0U, (uint16_t)(sizeof(dj_cal_music7) / 2U), 1U, dt); break;
        case 7: finish = beep_play_music_in_task(dj_cal_music8, 0U, (uint16_t)(sizeof(dj_cal_music8) / 2U), 1U, dt); break;
        default: break;
      }
      break;

    case BEEP_BLDC_ZERO_CAL:
      finish = beep_play_music_in_task(bldc_cal_zero_music, 0U, (uint16_t)(sizeof(bldc_cal_zero_music) / 2U), 1U, dt);
      break;

    case BEEP_BLDC_ZERO_INIT:
      finish = beep_play_music_in_task(bldc_cal_init_music, 0U, (uint16_t)(sizeof(bldc_cal_init_music) / 2U), 1U, dt);
      break;

    case BEEP_BLDC_GAIT_SWITCH:
      finish = beep_play_music_in_task(gait_switch_music, 0U, (uint16_t)(sizeof(gait_switch_music) / 2U), 1U, dt);
      break;

    case BEEP_BLDC_RESET_ERR:
      finish = beep_play_music_in_task(bldc_reset_err_music, 0U, (uint16_t)(sizeof(bldc_reset_err_music) / 2U), 1U, dt);
      break;

    case BEEP_BLDC_STATE:
      finish = beep_play_music_in_task(beep_bldc_state, 0U, (uint16_t)(sizeof(beep_bldc_state) / 2U), 1U, dt);
      break;

    default:
      beep_tone(0U, 0U);
      break;
  }

  return (char)finish;
}

void Beep_Init(uint32_t arr, uint32_t psc)
{
  GPIO_InitTypeDef gpio = {0};

  (void)arr;
  (void)psc;

  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_TIM4_CLK_ENABLE();

  gpio.Pin = BEEP_GPIO_PIN;
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pull = GPIO_PULLUP;
  gpio.Speed = GPIO_SPEED_FREQ_HIGH;
  gpio.Alternate = GPIO_AF2_TIM4;
  HAL_GPIO_Init(BEEP_GPIO_PORT, &gpio);

  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 84U - 1U;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 1000U - 1U;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

  if (HAL_TIM_PWM_Init(&htim4) != HAL_OK) {
    return;
  }

  TIM_OC_InitTypeDef oc = {0};
  oc.OCMode = TIM_OCMODE_PWM1;
  oc.Pulse = 0U;
  oc.OCPolarity = TIM_OCPOLARITY_LOW;
  oc.OCFastMode = TIM_OCFAST_DISABLE;

  if (HAL_TIM_PWM_ConfigChannel(&htim4, &oc, TIM_CHANNEL_2) != HAL_OK) {
    return;
  }

  if (HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_2) != HAL_OK) {
    return;
  }

  beep_initialized = 1U;
  beep_reset_task();

  /* Keep these legacy melodies linked as in old firmware. */
  (void)start_music_micro;
  (void)start_music_pi;
}

HAL_StatusTypeDef beep_service_start(void)
{
  if (!beep_initialized) {
    return HAL_ERROR;
  }

  if (beep_queue == NULL) {
    beep_queue = osMessageQueueNew(BEEP_QUEUE_LEN, sizeof(uint8_t), NULL);
    if (beep_queue == NULL) {
      return HAL_ERROR;
    }
  }

  if (beep_task_id == NULL) {
    const osThreadAttr_t beep_attr = {
      .name = "BEEP",
      .priority = osPriorityLow,
      .stack_size = 768
    };

    beep_task_id = osThreadNew(beep_task, NULL, &beep_attr);
    if (beep_task_id == NULL) {
      return HAL_ERROR;
    }
  }

  return HAL_OK;
}

HAL_StatusTypeDef beep_post(uint8_t melodyId)
{
  uint32_t free_slots;

  if ((beep_queue == NULL) || (beep_task_id == NULL)) {
    return HAL_ERROR;
  }

  free_slots = osMessageQueueGetSpace(beep_queue);
  if (free_slots == 0U) {
    return HAL_BUSY;
  }

  if (osMessageQueuePut(beep_queue, &melodyId, 0U, 0U) != osOK) {
    return HAL_BUSY;
  }

  return HAL_OK;
}

uint32_t beep_queue_free_slots(void)
{
  if (beep_queue == NULL) {
    return 0U;
  }

  return osMessageQueueGetSpace(beep_queue);
}
