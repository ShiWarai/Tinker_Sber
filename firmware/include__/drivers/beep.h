#ifndef BEEP_H
#define BEEP_H

#include <stdint.h>

enum {
	MEMS_RIGHT_BEEP = 0,
	MEMS_ERROR_BEEP = 1,
	START_BEEP = 2,
	BAT_ERO_BEEP = 3,
	RC_ERO_BEEP = 4,
	BEEP_ONE = 5,
	BEEP_TWO = 6,
	BEEP_THREE = 7,
	BEEP_STATE = 8,
	BEEP_GPS_SAVE = 9,
	BEEP_HML_CAL = 10,
	BEEP_MISSION = 11,
	MEMS_WAY_UPDATE = 12,
	MEMS_GPS_RIGHT = 14,
	BEEP_DJ_CAL1 = 15,
	BEEP_DJ_CAL2 = 16,
	BEEP_BLDC_ZERO_CAL = 17,
	BEEP_BLDC_ZERO_INIT = 18,
	BEEP_BLDC_GAIT_SWITCH = 19,
	BEEP_BLDC_STATE = 20,
	BEEP_BLDC_RESET_ERR = 21,
	BEEP_BLDC_SPI_CONNECT = 22,
	BEEP_BLDC_SPI_CONNECT_THREAD_UP = 23,
};

void beep_init(void);
void beep_startup_melody(void);
void beep_play_event(uint8_t event_id);
void beep_update(float dt_s);
void beep_stop(void);
void beep_on(uint32_t freq);
void beep_off(void);

#endif /* BEEP_H */

