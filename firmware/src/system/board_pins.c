#include "system/board_pins.h"

void board_gpio_init(void)
{
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  GPIO_InitTypeDef gpio = {0};

  gpio.Mode  = GPIO_MODE_OUTPUT_PP;
  gpio.Pull  = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_LOW;

  /* RED LED */
  gpio.Pin = LED_RED_PIN;
  HAL_GPIO_Init(LED_RED_PORT, &gpio);

  /* BLUE LED */
  gpio.Pin = LED_BLUE_PIN;
  HAL_GPIO_Init(LED_BLUE_PORT, &gpio);

  /* Status LEDs */
  gpio.Pin = LED_STATUS_RED_PIN;
  HAL_GPIO_Init(LED_STATUS_RED_PORT, &gpio);

  gpio.Pin = LED_STATUS_BLUE_PIN;
  HAL_GPIO_Init(LED_STATUS_BLUE_PORT, &gpio);

  /* Leg power control */
  gpio.Pin = LEG_POWER_PIN;
  HAL_GPIO_Init(LEG_POWER_PORT, &gpio);
}
