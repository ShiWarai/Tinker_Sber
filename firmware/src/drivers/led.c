#include "include.h"
#include "drivers/led.h"

void led_init(void)
{
  board_gpio_init();
}

void led_red(uint8_t on)
{
  HAL_GPIO_WritePin(LED_RED_PORT, LED_RED_PIN,
                    on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void led_blue(uint8_t on)
{
  HAL_GPIO_WritePin(LED_BLUE_PORT, LED_BLUE_PIN,
                    on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
