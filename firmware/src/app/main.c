#include "include.h"

static void fatal_delay_ms(uint32_t ms)
{
  for (uint32_t i = 0; i < ms; i++) {
    for (volatile uint32_t j = 0; j < 12000U; j++) {
      __NOP();
    }
  }
}

static void fatal_blink(uint32_t on_ms, uint32_t off_ms)
{
  for (;;) {
    led_red(1);
    led_blue(1);
    fatal_delay_ms(on_ms);
    led_red(0);
    led_blue(0);
    fatal_delay_ms(off_ms);
  }
}

static void task_led_red(void *argument)
{
  (void)argument;
  for (;;) {
    led_red(1);
    osDelay(250);
    led_red(0);
    osDelay(250);
  }
}

static void task_led_blue(void *argument)
{
  (void)argument;
  for (;;) {
    uint32_t on_ms;
    uint32_t off_ms;

    if (!can1_is_ready()) {
      on_ms = 50U;
      off_ms = 50U;
    } else {
      switch (can1_link_state()) {
        case CAN_LINK_RUN:
          on_ms = 100U;
          off_ms = 900U;
          break;
        case CAN_LINK_SAFE_STOP:
          on_ms = 500U;
          off_ms = 500U;
          break;
        default:
          on_ms = 200U;
          off_ms = 200U;
          break;
      }
    }

    led_blue(1);
    osDelay(on_ms);
    led_blue(0);
    osDelay(off_ms);
  }
}

static void task_can_service(void *argument)
{
  (void)argument;

  if (can1_service_start() != HAL_OK) {
    fatal_blink(100, 100);
  }

  for (;;) {
    can1_service_step_1ms();
    osDelay(1);
  }
}

static void task_can_test(void *argument)
{
  (void)argument;

  mit_command_t cmd = {
    .p  = 0.0f,
    .v  = 0.0f,
    .kp = 1.0f,
    .kd = 0.0f,
    .t  = 0.0f,
  };

  osDelay(2000);

  for (;;) {
    can1_set_enable_request(1);
    osDelay(1000);

    for (uint32_t t = 0; t < 150; t++) {
      for (uint8_t id = 1; id <= 10; id++) {
        can_queue_motor_command(id, &cmd);
      }
      osDelay(20);
    }

    can1_set_enable_request(0);
    osDelay(2000);
  }
}

static void init_beep(void)
{
  Beep_Init(0U, 83U);
  if (beep_service_start() != HAL_OK) {
    fatal_blink(80, 80);
  }

  (void)beep_post(START_BEEP);
}

static void task_init(void *argument)
{
  (void)argument;

  led_init();
  init_beep();

  if (can1_init() != HAL_OK) {
    fatal_blink(150, 150);
  }

  const osThreadAttr_t can_attr = {
    .name       = "CAN_SVC",
    .priority   = osPriorityHigh,
    .stack_size = 1024
  };

  const osThreadAttr_t red_attr = {
    .name       = "LED_RED",
    .priority   = osPriorityNormal,
    .stack_size = 512
  };

  const osThreadAttr_t blue_attr = {
    .name       = "LED_BLUE",
    .priority   = osPriorityNormal,
    .stack_size = 512
  };

  if (osThreadNew(task_can_service, NULL, &can_attr) == NULL) {
    fatal_blink(160, 160);
  }

  const osThreadAttr_t test_attr = {
    .name       = "CAN_TST",
    .priority   = osPriorityNormal,
    .stack_size = 512
  };

  if (osThreadNew(task_can_test, NULL, &test_attr) == NULL) {
    fatal_blink(170, 170);
  }

  osThreadId_t red_id = osThreadNew(task_led_red, NULL, &red_attr);
  osThreadId_t blue_id = osThreadNew(task_led_blue, NULL, &blue_attr);
  if ((red_id == NULL) || (blue_id == NULL)) {
    fatal_blink(120, 120);
  }

  osThreadExit();
}

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  if (osKernelInitialize() != osOK) {
    fatal_blink(60, 60);
  }

  const osThreadAttr_t init_attr = {
    .name       = "INIT",
    .priority   = osPriorityAboveNormal,
    .stack_size = 512
  };

  if (osThreadNew(task_init, NULL, &init_attr) == NULL) {
    fatal_blink(120, 120);
  }

  if (osKernelStart() != osOK) {
    fatal_blink(200, 200);
  }

  /* Сюда управление не должно вернуться */
  for (;;) {}
}
