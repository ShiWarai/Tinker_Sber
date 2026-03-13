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
    led_blue(1);
    osDelay(500);
    led_blue(0);
    osDelay(500);
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
