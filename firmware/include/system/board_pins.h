#ifndef BOARD_PINS_H
#define BOARD_PINS_H

#include "stm32f4xx_hal.h"

/* LED pins (PB3/PB4 RED/BLUE, PA0/PA1 status, PC5 leg power) */
#define LED_RED_PORT          GPIOB
#define LED_RED_PIN           GPIO_PIN_3
#define LED_BLUE_PORT         GPIOB
#define LED_BLUE_PIN          GPIO_PIN_4
#define LED_STATUS_RED_PORT   GPIOA
#define LED_STATUS_RED_PIN    GPIO_PIN_0
#define LED_STATUS_BLUE_PORT  GPIOA
#define LED_STATUS_BLUE_PIN   GPIO_PIN_1
#define LED_SCL_PORT          GPIOA
#define LED_SCL_PIN           GPIO_PIN_6
#define LED_SCP_PORT          GPIOA
#define LED_SCP_PIN           GPIO_PIN_7
#define LEG_POWER_PORT        GPIOC
#define LEG_POWER_PIN         GPIO_PIN_5

/* Button (KEY_DOG) - PB12 */
#define BTN_DOG_PORT          GPIOB
#define BTN_DOG_PIN           GPIO_PIN_12

void board_gpio_init(void);

#endif /* BOARD_PINS_H */
