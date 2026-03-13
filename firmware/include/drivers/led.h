#ifndef LED_H
#define LED_H

#include <stdint.h>

void led_init(void);
void led_red(uint8_t on);
void led_blue(uint8_t on);

#endif /* LED_H */
