#ifndef BOARD_PINS_H
#define BOARD_PINS_H

#include "stm32f4xx_hal.h"

/* LED pins (from legacy led_fc.c: PB3/PB4 RED/BLUE, PA0/PA1 status, PC5 leg power) */
#define LED_RED_PORT    GPIOB
#define LED_RED_PIN    GPIO_PIN_3
#define LED_BLUE_PORT   GPIOB
#define LED_BLUE_PIN   GPIO_PIN_4
#define LED_STATUS_RED_PORT  GPIOA
#define LED_STATUS_RED_PIN   GPIO_PIN_0
#define LED_STATUS_BLUE_PORT GPIOA
#define LED_STATUS_BLUE_PIN  GPIO_PIN_1
#define LED_SCL_PORT    GPIOA
#define LED_SCL_PIN     GPIO_PIN_6
#define LED_SCP_PORT    GPIOA
#define LED_SCP_PIN     GPIO_PIN_7
#define LEG_POWER_PORT  GPIOC
#define LEG_POWER_PIN   GPIO_PIN_5

/* Button (KEY_DOG) - PB12 */
#define BTN_DOG_PORT    GPIOB
#define BTN_DOG_PIN     GPIO_PIN_12

/* SPI3 for External Flash & Sensors */
#define SPI3_SCK_PORT   GPIOC
#define SPI3_SCK_PIN    GPIO_PIN_10
#define SPI3_MISO_PORT  GPIOC
#define SPI3_MISO_PIN   GPIO_PIN_11
#define SPI3_MOSI_PORT  GPIOC
#define SPI3_MOSI_PIN   GPIO_PIN_12

#define FLASH_CS_PORT   GPIOA
#define FLASH_CS_PIN    GPIO_PIN_8

#define ICM_CS_PORT     GPIOA
#define ICM_CS_PIN      GPIO_PIN_5

/* Buzzer - PB7, TIM4_CH2 */
#define BEEP_PORT       GPIOB
#define BEEP_PIN        GPIO_PIN_7

/* CAN1 (legacy: PB8 RX, PB9 TX) */
#define CAN1_RX_PORT    GPIOB
#define CAN1_RX_PIN     GPIO_PIN_8
#define CAN1_TX_PORT    GPIOB
#define CAN1_TX_PIN     GPIO_PIN_9

/* CAN2 (legacy: PB5 RX, PB6 TX) */
#define CAN2_RX_PORT    GPIOB
#define CAN2_RX_PIN     GPIO_PIN_5
#define CAN2_TX_PORT    GPIOB
#define CAN2_TX_PIN     GPIO_PIN_6

void board_gpio_init(void);

#endif /* BOARD_PINS_H */
