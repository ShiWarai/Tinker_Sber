#ifndef LED_H
#define LED_H

#include <stdint.h>

#define LED_RED    0
#define LED_BLUE   1

void led_init(void);
void led_red(uint8_t on);
void led_blue(uint8_t on);
void led_status_red(uint8_t on);
void led_status_blue(uint8_t on);
void led_scl(uint8_t on);
void led_scp(uint8_t on);

void led_status_update(float dt);

#endif /* LED_H */
