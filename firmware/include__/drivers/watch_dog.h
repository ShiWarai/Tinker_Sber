#ifndef WATCHDOG_H
#define WATCHDOG_H

#include "stm32f4xx_hal.h"

void watchdog_init(uint32_t timeout_ms);
void watchdog_feed(void);

uint8_t button_dog_read(void);

#endif /* WATCHDOG_H */
