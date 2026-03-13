#include "watch_dog.h"
#include "board_pins.h"

IWDG_HandleTypeDef hiwdg;

/**
 * @brief Initialize Independent Watchdog
 * @param timeout_ms Timeout in milliseconds (approximate)
 * 
 * LSI is ~32kHz. Prescaler 32 gives 1kHz clock.
 * RL value is timeout in ms. Max RL is 4095 (4 seconds).
 */
void watchdog_init(uint32_t timeout_ms) {
    if (timeout_ms > 4095) timeout_ms = 4095;
    
    hiwdg.Instance = IWDG;
    hiwdg.Init.Prescaler = IWDG_PRESCALER_32;
    hiwdg.Init.Reload = timeout_ms;
    HAL_IWDG_Init(&hiwdg);
}

void watchdog_feed(void) {
    HAL_IWDG_Refresh(&hiwdg);
}

/**
 * @brief Read the Dog/User button state
 * @return 1 if pressed, 0 otherwise
 */
uint8_t button_dog_read(void) {
    // BTN_DOG_PIN (PB12) is often active-low if pulled up
    return (HAL_GPIO_ReadPin(BTN_DOG_PORT, BTN_DOG_PIN) == GPIO_PIN_RESET) ? 1 : 0;
}
