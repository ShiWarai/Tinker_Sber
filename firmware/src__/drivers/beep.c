#include "stm32f4xx_hal.h"
#include "board_pins.h"
#include "beep.h"
#include "beep_melodies.h"

typedef struct {
  const uint8_t *data;
  uint16_t note_count;
} beep_melody_t;

typedef struct {
  const beep_melody_t *melody;
  uint16_t index;
  float time_left_s;
  uint8_t active;
} beep_player_t;

static TIM_HandleTypeDef htim4;
static beep_player_t beep_player;

static const uint16_t beat_delay_ms[] = {0U, 62U, 94U, 125U, 125U, 187U, 250U};
static const uint16_t tone_table[3][8] = {
  {0U, 261U, 293U, 329U, 349U, 391U, 440U, 493U},
  {0U, 523U, 587U, 659U, 698U, 783U, 880U, 987U},
  {0U, 1046U, 1174U, 1318U, 1396U, 1567U, 1760U, 1975U},
};

static const beep_melody_t melody_start_px4 = {
  beep_melody_start_px4,
  (uint16_t)(sizeof(beep_melody_start_px4) / 2U),
};
static const beep_melody_t melody_start_pi = {
  beep_melody_start_pi,
  (uint16_t)(sizeof(beep_melody_start_pi) / 2U),
};
static const beep_melody_t melody_start_pi_thread = {
  beep_melody_start_pi_thread,
  (uint16_t)(sizeof(beep_melody_start_pi_thread) / 2U),
};
static const beep_melody_t melody_start_windows = {
  beep_melody_start_windows,
  (uint16_t)(sizeof(beep_melody_start_windows) / 2U),
};
static const beep_melody_t melody_mems_gps = {
  beep_melody_mems_gps,
  (uint16_t)(sizeof(beep_melody_mems_gps) / 2U),
};
static const beep_melody_t melody_mems_right = {
  beep_melody_mems_right,
  (uint16_t)(sizeof(beep_melody_mems_right) / 2U),
};
static const beep_melody_t melody_mems_error = {
  beep_melody_mems_error,
  (uint16_t)(sizeof(beep_melody_mems_error) / 2U),
};
static const beep_melody_t melody_mission = {
  beep_melody_mission,
  (uint16_t)(sizeof(beep_melody_mission) / 2U),
};
static const beep_melody_t melody_mems_wayup = {
  beep_melody_mems_wayup,
  (uint16_t)(sizeof(beep_melody_mems_wayup) / 2U),
};
static const beep_melody_t melody_bat_error = {
  beep_melody_bat_error,
  (uint16_t)(sizeof(beep_melody_bat_error) / 2U),
};
static const beep_melody_t melody_rc_error = {
  beep_melody_rc_error,
  (uint16_t)(sizeof(beep_melody_rc_error) / 2U),
};
static const beep_melody_t melody_hml_cal = {
  beep_melody_hml_cal,
  (uint16_t)(sizeof(beep_melody_hml_cal) / 2U),
};
static const beep_melody_t melody_bldc_zero_cal = {
  beep_melody_bldc_zero_cal,
  (uint16_t)(sizeof(beep_melody_bldc_zero_cal) / 2U),
};
static const beep_melody_t melody_bldc_zero_init = {
  beep_melody_bldc_zero_init,
  (uint16_t)(sizeof(beep_melody_bldc_zero_init) / 2U),
};
static const beep_melody_t melody_bldc_reset_err = {
  beep_melody_bldc_reset_err,
  (uint16_t)(sizeof(beep_melody_bldc_reset_err) / 2U),
};
static const beep_melody_t melody_gait_switch = {
  beep_melody_gait_switch,
  (uint16_t)(sizeof(beep_melody_gait_switch) / 2U),
};
static const beep_melody_t melody_dj_cal[] = {
  {beep_melody_dj_cal_1, (uint16_t)(sizeof(beep_melody_dj_cal_1) / 2U)},
  {beep_melody_dj_cal_2, (uint16_t)(sizeof(beep_melody_dj_cal_2) / 2U)},
  {beep_melody_dj_cal_3, (uint16_t)(sizeof(beep_melody_dj_cal_3) / 2U)},
  {beep_melody_dj_cal_4, (uint16_t)(sizeof(beep_melody_dj_cal_4) / 2U)},
  {beep_melody_dj_cal_5, (uint16_t)(sizeof(beep_melody_dj_cal_5) / 2U)},
  {beep_melody_dj_cal_6, (uint16_t)(sizeof(beep_melody_dj_cal_6) / 2U)},
  {beep_melody_dj_cal_7, (uint16_t)(sizeof(beep_melody_dj_cal_7) / 2U)},
  {beep_melody_dj_cal_8, (uint16_t)(sizeof(beep_melody_dj_cal_8) / 2U)},
};

static void beep_set_frequency(uint32_t freq_hz)
{
  if (freq_hz == 0U) {
    HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_2);
    return;
  }

  uint32_t timer_clk = 84000000U;
  uint32_t prescaler = 84U - 1U; /* 1 MHz timer clock */
  uint32_t period = timer_clk / (prescaler + 1U) / freq_hz;
  if (period == 0U) {
    period = 1U;
  }

  __HAL_TIM_DISABLE(&htim4);
  __HAL_TIM_SET_PRESCALER(&htim4, prescaler);
  __HAL_TIM_SET_AUTORELOAD(&htim4, period - 1U);
  __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_2, period / 2U);
  __HAL_TIM_SET_COUNTER(&htim4, 0U);
  __HAL_TIM_ENABLE(&htim4);

  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_2);
}

static uint32_t beep_frequency_from_code(uint8_t note_code)
{
  uint8_t level = note_code / 10U;
  uint8_t tone = note_code % 10U;

  if ((level >= 3U) || (tone >= 8U)) {
    return 0U;
  }

  return tone_table[level][tone];
}

static float beep_duration_from_code(uint8_t beat_code)
{
  uint8_t beat = beat_code / 10U;
  uint8_t loops = beat_code % 10U;

  if ((beat >= (sizeof(beat_delay_ms) / sizeof(beat_delay_ms[0]))) || (loops == 0U)) {
    return 0.0f;
  }

  return ((float)beat_delay_ms[beat] * (float)loops) / 1000.0f;
}

static void beep_apply_note(uint8_t note_code)
{
  beep_set_frequency(beep_frequency_from_code(note_code));
}

static void beep_advance_step(void)
{
  if ((beep_player.melody == NULL) || (beep_player.index >= beep_player.melody->note_count)) {
    beep_stop();
    return;
  }

  uint16_t offset = (uint16_t)(beep_player.index * 2U);
  beep_apply_note(beep_player.melody->data[offset]);
  beep_player.time_left_s = beep_duration_from_code(beep_player.melody->data[offset + 1U]);
  beep_player.index++;

  if (beep_player.time_left_s <= 0.0f) {
    beep_player.time_left_s = 0.001f;
  }
}

static void beep_start_melody(const beep_melody_t *melody)
{
  if ((melody == NULL) || (melody->data == NULL) || (melody->note_count == 0U)) {
    beep_stop();
    return;
  }

  beep_player.melody = melody;
  beep_player.index = 0U;
  beep_player.time_left_s = 0.0f;
  beep_player.active = 1U;
  beep_advance_step();
}

static const beep_melody_t *beep_lookup_melody(uint8_t event_id)
{
  switch (event_id) {
    case MEMS_RIGHT_BEEP:
      return &melody_mems_right;
    case MEMS_ERROR_BEEP:
      return &melody_mems_error;
    case START_BEEP:
      return &melody_start_windows;
    case BAT_ERO_BEEP:
      return &melody_bat_error;
    case RC_ERO_BEEP:
      return &melody_rc_error;
    case BEEP_HML_CAL:
      return &melody_hml_cal;
    case BEEP_MISSION:
      return &melody_mission;
    case MEMS_WAY_UPDATE:
      return &melody_mems_wayup;
    case MEMS_GPS_RIGHT:
      return &melody_mems_gps;
    case BEEP_DJ_CAL1:
      return &melody_dj_cal[0];
    case BEEP_DJ_CAL2:
      return &melody_dj_cal[1];
    case BEEP_BLDC_ZERO_CAL:
      return &melody_bldc_zero_cal;
    case BEEP_BLDC_ZERO_INIT:
      return &melody_bldc_zero_init;
    case BEEP_BLDC_GAIT_SWITCH:
      return &melody_gait_switch;
    case BEEP_BLDC_RESET_ERR:
      return &melody_bldc_reset_err;
    case BEEP_BLDC_SPI_CONNECT:
      return &melody_start_pi;
    case BEEP_BLDC_SPI_CONNECT_THREAD_UP:
      return &melody_start_pi_thread;
    default:
      return NULL;
  }
}

static void beep_play_blocking(const beep_melody_t *melody)
{
  if ((melody == NULL) || (melody->data == NULL)) {
    return;
  }

  for (uint16_t i = 0U; i < melody->note_count; i++) {
    uint16_t offset = (uint16_t)(i * 2U);
    beep_apply_note(melody->data[offset]);
    HAL_Delay((uint32_t)(beep_duration_from_code(melody->data[offset + 1U]) * 1000.0f));
  }

  beep_stop();
}

void beep_init(void)
{
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_TIM4_CLK_ENABLE();

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = BEEP_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF2_TIM4;
  HAL_GPIO_Init(BEEP_PORT, &GPIO_InitStruct);

  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 84U - 1U; /* 1 MHz */
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 1000U - 1U;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  HAL_TIM_PWM_Init(&htim4);

  TIM_OC_InitTypeDef sConfigOC = {0};
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = (htim4.Init.Period + 1U) / 2U;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_2);

  HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_2);
}

void beep_startup_melody(void)
{
  beep_play_blocking(&melody_start_px4);
}

void beep_play_event(uint8_t event_id)
{
  const beep_melody_t *melody = beep_lookup_melody(event_id);
  if (melody != NULL) {
    beep_start_melody(melody);
  }
}

void beep_update(float dt_s)
{
  if (!beep_player.active) {
    return;
  }

  if (dt_s < 0.0f) {
    return;
  }

  while (beep_player.active && (dt_s >= beep_player.time_left_s)) {
    dt_s -= beep_player.time_left_s;
    if (beep_player.index >= beep_player.melody->note_count) {
      beep_stop();
      return;
    }
    beep_advance_step();
  }

  if (beep_player.active) {
    beep_player.time_left_s -= dt_s;
  }
}

void beep_stop(void)
{
  beep_player.active = 0U;
  beep_player.melody = NULL;
  beep_player.index = 0U;
  beep_player.time_left_s = 0.0f;
  beep_set_frequency(0U);
}

void beep_on(uint32_t freq)
{
  beep_player.active = 0U;
  beep_set_frequency(freq);
}

void beep_off(void)
{
  beep_stop();
}

