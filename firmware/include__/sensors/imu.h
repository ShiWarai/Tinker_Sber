#ifndef _IMU_H_
#define _IMU_H_

#include <stdint.h>
#include "app_main.h"

// IMU Attitude state (Global)
extern float Roll, Pitch, Yaw;

// Functions
void IMU_update(float T, float gx, float gy, float gz, float ax, float ay, float az);
float invSqrt(float x);

#endif /* _IMU_H_ */
