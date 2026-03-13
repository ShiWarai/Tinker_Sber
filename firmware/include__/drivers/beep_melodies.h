#ifndef BEEP_MELODIES_H
#define BEEP_MELODIES_H

#include <stdint.h>

static const uint8_t beep_melody_start_px4[] = {
  13,12, 16,12, 15,12, 13,12, 16,12, 15,12,
  13,12, 16,12, 15,12, 13,12, 16,12, 15,12,
  16,12, 15,11, 16,11, 15,11, 16,11, 15,43,
};

static const uint8_t beep_melody_start_pi[] = {
  27,1, 0,1, 0,59, 0,59,
};

static const uint8_t beep_melody_start_pi_thread[] = {
  27,1, 0,1, 17,1, 0,1, 0,59, 0,59,
};

static const uint8_t beep_melody_start_windows[] = {
  21,21, 1,11, 21,21, 1,11, 21,21, 1,11,
  11,13, 15,13, 13,11, 1,11, 13,11, 1,11, 13,11, 1,11,
  21,23, 15,41, 1,11, 15,31, 1,11, 15,21, 1,11, 15,11, 1,11,
};

static const uint8_t beep_melody_mems_gps[] = {
  15,44, 17,33, 17,33,
};

static const uint8_t beep_melody_mems_right[] = {
  5,64, 0,13, 12,64,
};

static const uint8_t beep_melody_mems_error[] = {
  2,34, 0,32, 2,34, 0,32,
};

static const uint8_t beep_melody_mission[] = {
  22,35, 22,35, 22,65,
};

static const uint8_t beep_melody_mems_wayup[] = {
  15,52, 17,32, 17,32,
};

static const uint8_t beep_melody_bat_error[] = {
  7,33, 0,32, 7,33, 0,32, 7,33, 0,32, 7,33, 0,32,
  7,33, 0,32, 7,33, 0,32, 7,33, 0,32, 7,33, 0,32,
  7,33, 0,32, 7,33, 0,32, 7,33, 0,32, 7,33, 0,32,
  7,33, 0,32, 7,33, 0,32, 7,33, 0,32, 7,33, 0,32,
};

static const uint8_t beep_melody_rc_error[] = {
  27,11, 0,11, 27,11, 0,11, 27,11, 0,11, 0,39, 0,39,
};

static const uint8_t beep_melody_hml_cal[] = {
  13,11, 0,11, 13,11, 0,11, 13,11, 0,11, 0,39, 0,39,
};

static const uint8_t beep_melody_bldc_zero_cal[] = {
  5,64, 0,13, 12,64,
};

static const uint8_t beep_melody_bldc_zero_init[] = {
  3,64, 0,13, 15,34,
};

static const uint8_t beep_melody_bldc_reset_err[] = {
  2,34, 0,32, 2,34, 0,32,
};

static const uint8_t beep_melody_gait_switch[] = {
  15,14, 0,13, 12,14,
};

static const uint8_t beep_melody_dj_cal_1[] = {
  27,1, 0,1, 0,39, 0,39,
};

static const uint8_t beep_melody_dj_cal_2[] = {
  27,1, 0,1, 27,1, 0,1, 0,39, 0,39,
};

static const uint8_t beep_melody_dj_cal_3[] = {
  27,1, 0,1, 27,1, 0,1, 27,1, 0,1, 0,39, 0,39,
};

static const uint8_t beep_melody_dj_cal_4[] = {
  27,1, 0,1, 27,1, 0,1, 27,1, 0,1, 27,1, 0,1, 0,39, 0,39,
};

static const uint8_t beep_melody_dj_cal_5[] = {
  27,1, 0,1, 27,1, 0,1, 27,1, 0,1, 27,1, 0,1, 27,1, 0,1, 0,39, 0,39,
};

static const uint8_t beep_melody_dj_cal_6[] = {
  27,1, 0,1, 27,1, 0,1, 27,1, 0,1, 27,1, 0,1, 27,1, 0,1, 27,1, 0,1, 0,39, 0,39,
};

static const uint8_t beep_melody_dj_cal_7[] = {
  27,1, 0,1, 27,1, 0,1, 27,1, 0,1, 27,1, 0,1, 27,1, 0,1, 27,1, 0,1, 27,1, 0,1, 0,39, 0,39,
};

static const uint8_t beep_melody_dj_cal_8[] = {
  27,1, 0,1, 27,1, 0,1, 27,1, 0,1, 27,1, 0,1, 27,1, 0,1, 27,1, 0,1, 27,1, 0,1, 27,1, 0,1, 0,39, 0,39,
};

#endif /* BEEP_MELODIES_H */