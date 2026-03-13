#include "led.h"
#include "board_pins.h"
#include "stm32f4xx_hal.h"
#include "app_main.h"

void led_init(void)
{
  board_gpio_init();

  // Короткая стартовая индикация, чтобы было видно перезапуск контроллера.
  for (int i = 0; i < 2; i++) {
    led_red(1);
    led_blue(1);
    led_status_red(1);
    led_status_blue(1);
    HAL_Delay(120);

    led_red(0);
    led_blue(0);
    led_status_red(0);
    led_status_blue(0);
    HAL_Delay(120);
  }
}

void led_red(uint8_t on)
{
  HAL_GPIO_WritePin(LED_RED_PORT, LED_RED_PIN, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void led_blue(uint8_t on)
{
  HAL_GPIO_WritePin(LED_BLUE_PORT, LED_BLUE_PIN, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void led_status_red(uint8_t on)
{
  HAL_GPIO_WritePin(LED_STATUS_RED_PORT, LED_STATUS_RED_PIN, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void led_status_blue(uint8_t on)
{
  HAL_GPIO_WritePin(LED_STATUS_BLUE_PORT, LED_STATUS_BLUE_PIN, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void led_scl(uint8_t on)
{
  HAL_GPIO_WritePin(LED_SCL_PORT, LED_SCL_PIN, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void led_scp(uint8_t on)
{
  HAL_GPIO_WritePin(LED_SCP_PORT, LED_SCP_PIN, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

extern int can1_rx_cnt;
extern int can2_rx_cnt;
extern uint8_t spi_master_connect_pi;
extern motor_measure_t motor_chassis[10];

void led_status_update(float dt)
{
  static uint32_t timer = 0;
  timer++;

  // LED_BLUE: CAN Motors Connectivity
  int can_connected_count = 0;
  for (int i = 0; i < 10; i++) {
    if (motor_chassis[i].param.connect) can_connected_count++;
  }
  
  if (can_connected_count > 0) {
    if (timer % 20 < 10) led_blue(1); else led_blue(0);
  } else {
    led_blue(0);
  }

  // LED_RED: SPI Link Status (Blink if connected to RPI)
  if (spi_master_connect_pi) {
    if (timer % 50 < 25) led_red(1); else led_red(0);
  } else {
    led_red(0);
  }
}

